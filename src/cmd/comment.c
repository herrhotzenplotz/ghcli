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
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	version();
	copyright();
}

static void
comment_init(struct gcli_ctx *ctx, FILE *f, void *_data)
{
	struct gcli_submit_comment_opts *info = _data;
	const char *target_type = NULL;

	switch (info->target_type) {
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

	fprintf(
		f,
		"! Enter your comment above, save and exit.\n"
		"! All lines with a leading '!' are discarded and will not\n"
		"! appear in your comment.\n"
		"! COMMENT IN : %s/%s %s #%"PRIid"\n",
		info->owner, info->repo, target_type, info->target_id);
}

static char *
gcli_comment_get_message(struct gcli_submit_comment_opts *info)
{
	return gcli_editor_get_user_message(g_clictx, comment_init, info);
}

static int
comment_submit(struct gcli_submit_comment_opts opts, int always_yes)
{
	int rc = 0;
	char *message;

	message = gcli_comment_get_message(&opts);
	opts.message = message;

	if (message == NULL)
		errx(1, "gcli: empty message. aborting.");

	fprintf(
		stdout,
		"You will be commenting the following in %s/%s #%"PRIid":\n%s\n",
		opts.owner, opts.repo, opts.target_id, opts.message);

	if (!always_yes) {
		if (!sn_yesno("Is this okay?"))
			errx(1, "Aborted by user");
	}

	rc = gcli_comment_submit(g_clictx, &opts);

	free(message);
	opts.message = NULL;

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
		pretty_print(list->comments[i].body, 9, 80, stdout);
		putchar('\n');
	}
}

int
subcommand_comment(int argc, char *argv[])
{
	int ch, target_id = -1, rc = 0;
	char const *repo = NULL, *owner = NULL;
	bool always_yes = false;
	enum comment_target_type target_type;

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
		{0},
	};

	while ((ch = getopt_long(argc, argv, "yr:o:i:p:", options, NULL)) != -1) {
		switch (ch) {
		case 'r':
			repo = optarg;
			break;
		case 'o':
			owner = optarg;
			break;
		case 'p':
			target_type = PR_COMMENT;
			goto parse_target_id;
		case 'i':
			target_type = ISSUE_COMMENT;
		parse_target_id: {
				char *endptr;
				target_id = strtoul(optarg, &endptr, 10);
				if (endptr != optarg + strlen(optarg))
					err(1, "gcli: error: Cannot parse issue/PR number");
			} break;
		case 'y':
			always_yes = true;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&owner, &repo);

	if (target_id < 0) {
		fprintf(stderr, "gcli: error: missing issue/PR number (use -i/-p)\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = comment_submit((struct gcli_submit_comment_opts) {
			.owner = owner,
			.repo = repo,
			.target_type = target_type,
			.target_id = target_id },
		always_yes);

	if (rc < 0)
		errx(1, "gcli: error: failed to submit comment: %s", gcli_get_error(g_clictx));

	return EXIT_SUCCESS;
}
