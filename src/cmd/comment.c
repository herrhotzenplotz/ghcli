/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/comment.h>
#include <gcli/cmd/editor.h>

#include <gcli/comments.h>
#include <gcli/json_util.h>

#include <assert.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli comment [-o owner -r repo] [-p pr | -i issue] [-y]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -p pr           PR id to comment under\n");
	fprintf(stderr, "  -i issue        issue id to comment under\n");
	fprintf(stderr, "  -R comment-id   Reply to the comment with the given ID\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	version();
	copyright();
}

struct submit_ctx {
	struct gcli_submit_comment_opts opts;
	struct gcli_comment reply_comment;
};

void
gcli_print_prefixed(FILE *f, char const *const text, char const *const prefix)
{
	char const *bol = text;

	while (bol) {
		char const *const eol = strchr(bol, '\n');
		size_t const len = eol ? (size_t)(eol - bol) : strlen(bol);

		fprintf(f, "%s%.*s\n", prefix, (int)len, bol);

		if (!eol)
			break;

		bol = eol + 1;
	}
}

static void
comment_init(struct gcli_ctx *ctx, FILE *f, void *_data)
{
	struct submit_ctx *sctx = _data;
	const char *target_type = NULL;

	switch (sctx->opts.target_type) {
	case ISSUE_COMMENT:
		target_type = "issue";
		break;
	case PR_COMMENT: {
		switch (gcli_config_get_forge_type(ctx)) {
		case GCLI_FORGE_GITEA:
		case GCLI_FORGE_GITHUB:
			target_type = "Pull Request";
			break;
		case GCLI_FORGE_GITLAB:
			target_type = "Merge Request";
			break;
		case GCLI_FORGE_BUGZILLA:
			/* FIXME think about this one */
			assert(0 && "unreachable");
			break;
		}
	} break;
	}

	/* In case we reply to a comment, put the comment prefixed with
	 * '> ' into the file first. */
	if (sctx->reply_comment.body)
		gcli_print_prefixed(f, sctx->reply_comment.body, "> ");

	fprintf(
		f,
		"! Enter your comment above, save and exit.\n"
		"! All lines with a leading '!' are discarded and will not\n"
		"! appear in your comment.\n"
		"! COMMENT IN : %s/%s %s #%"PRIid"\n",
		sctx->opts.owner, sctx->opts.repo, target_type, sctx->opts.target_id);
}

static char *
gcli_comment_get_message(struct submit_ctx *info)
{
	return gcli_editor_get_user_message(g_clictx, comment_init, info);
}

static int
comment_submit(struct submit_ctx *sctx, int always_yes)
{
	int rc = 0;
	char *message;

	message = gcli_comment_get_message(sctx);
	sctx->opts.message = message;

	if (message == NULL)
		errx(1, "gcli: empty message. aborting.");

	fprintf(
		stdout,
		"You will be commenting the following in %s/%s #%"PRIid":\n%s\n",
		sctx->opts.owner, sctx->opts.repo, sctx->opts.target_id, sctx->opts.message);

	if (!always_yes) {
		if (!sn_yesno("Is this okay?"))
			errx(1, "Aborted by user");
	}

	rc = gcli_comment_submit(g_clictx, &sctx->opts);

	free(message);
	sctx->opts.message = NULL;

	return rc;
}

int
gcli_issue_comments(char const *owner, char const *repo, int const issue)
{
	struct gcli_comment_list list = {0};
	int rc = 0;

	rc = gcli_get_issue_comments(g_clictx, owner, repo, issue, &list);
	if (rc < 0)
		return rc;

	gcli_print_comment_list(&list);
	gcli_comments_free(&list);

	return rc;
}

int
gcli_pull_comments(char const *owner, char const *repo, int const pull)
{
	struct gcli_comment_list list = {0};
	int rc = 0;

	rc = gcli_get_pull_comments(g_clictx, owner, repo, pull, &list);
	if (rc < 0)
		return rc;

	gcli_print_comment_list(&list);
	gcli_comments_free(&list);

	return rc;
}

void
gcli_print_comment_list(struct gcli_comment_list const *const list)
{
	for (size_t i = 0; i < list->comments_size; ++i) {
		printf("AUTHOR : %s%s%s\n"
		       "DATE   : %s\n"
		       "ID     : %"PRIid"\n",
		       gcli_setbold(), list->comments[i].author, gcli_resetbold(),
		       list->comments[i].date,
		       list->comments[i].id);
		gcli_pretty_print(list->comments[i].body, 9, 80, stdout);
		putchar('\n');
	}
}

int
subcommand_comment(int argc, char *argv[])
{
	int ch, rc = 0;
	struct submit_ctx sctx = {0};
	bool always_yes = false;
	gcli_id reply_to_id = 0;

	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "issue",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{ .name    = "pull",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{ .name    = "in-reply-to",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'R' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "yr:o:i:p:R:", options, NULL)) != -1) {
		switch (ch) {
		case 'r':
			 sctx.opts.repo = optarg;
			break;
		case 'o':
			sctx.opts.owner = optarg;
			break;
		case 'p':
			sctx.opts.target_type = PR_COMMENT;
			goto parse_target_id;
		case 'i':
			sctx.opts.target_type = ISSUE_COMMENT;
		parse_target_id: {
				char *endptr;
				sctx.opts.target_id = strtoul(optarg, &endptr, 10);
				if (endptr != optarg + strlen(optarg))
					err(1, "gcli: error: Cannot parse issue/PR number");
			} break;
		case 'y':
			always_yes = true;
			break;
		case 'R': {
			char *endptr = NULL;
			reply_to_id = strtoul(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				err(1, "gcli: error: cannot parse comment id");
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&sctx.opts.owner, &sctx.opts.repo);

	if (!sctx.opts.target_id) {
		fprintf(stderr, "gcli: error: missing issue/PR number (use -i/-p)\n");
		usage();
		return EXIT_FAILURE;
	}

	if (reply_to_id) {
		rc = gcli_get_comment(g_clictx, sctx.opts.owner, sctx.opts.repo,
                              sctx.opts.target_type, sctx.opts.target_id,
		                      reply_to_id, &sctx.reply_comment);

		if (rc < 0) {
			errx(1, "gcli: error: failed to fetch comment for reply: %s",
			     gcli_get_error(g_clictx));
		}
	}

	rc = comment_submit(&sctx, always_yes);

	gcli_comment_free(&sctx.reply_comment);

	if (rc < 0)
		errx(1, "gcli: error: failed to submit comment: %s", gcli_get_error(g_clictx));

	return EXIT_SUCCESS;
}
