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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/editor.h>
#include <gcli/diffutil.h>
#include <gcli/pulls.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pulls review [-o owner -r repo] -i id\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "   -o owner         Operate on a repository by the given owner\n");
	fprintf(stderr, "   -r repo          Operate on the given repository\n");
	fprintf(stderr, "   -i id            Operate on the given PR id\n");
	fprintf(stderr, "\n");
	version();
}

static char *
get_review_file_cache_dir(void)
{
	/* FIXME */
	return sn_asprintf("%s/.cache/gcli/reviews",
	                   getenv("HOME"));
}

unsigned long
djb2(unsigned char const *str)
{
    unsigned long hash = 5381;
    int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;

	return hash;
}


static char *
make_review_diff_file_name(char const *const owner, char const *const repo,
                           gcli_id const pull_id)
{
	unsigned long hash = 0;

	hash ^= djb2((unsigned char const *)owner);
	hash ^= djb2((unsigned char const *)repo);

	return sn_asprintf("%lx_%lu.diff", hash, pull_id);
}

static char *
get_review_diff_file_name(char const *const owner, char const *const repo,
                          gcli_id const pull_id)
{
	char *base = get_review_file_cache_dir();
	char *file = make_review_diff_file_name(owner, repo, pull_id);
	char *path = sn_asprintf("%s/%s", base, file);
	free(base);
	free(file);

	return path;
}

struct review_ctx {
	char *diff_path;
	struct gcli_pull_create_review_details details;
};

static void
fetch_patch(struct review_ctx *ctx)
{
	/* diff does not exist, fetch it! */
	FILE *f = fopen(ctx->diff_path, "w");
	if (f == NULL)
		err(1, "error: cannot open %s", ctx->diff_path);

	if (gcli_pull_get_patch(g_clictx, f, ctx->details.owner, ctx->details.repo, ctx->details.pull_id) < 0) {
		errx(1, "error: failed to get patch: %s",
		     gcli_get_error(g_clictx));
	}

	fclose(f);
	f = NULL;
}

/* Splits the prelude of a patch series into two parts:
 *
 *  Lines starting with »GCLI: «
 *  Lines not starting with this prefix.
 *
 * Lines starting with this prefix will be stored in a meta */
static void
process_series_prelude(char *prelude, struct gcli_pull_create_review_details *details)
{
	char *meta, *comment, *bol;
	size_t const p_len = strlen(prelude);

	bol = prelude;

	meta = calloc(p_len + 1, 1);
	comment = calloc(p_len + 1, 1);

	/* Loop through input line-by-line */
	for (;;) {
		char *eol = strchr(bol, '\n');
		size_t line_len;
		static char const gcli_pref[] = "GCLI: ";
		static size_t const gcli_pref_len = sizeof(gcli_pref) - 1;

		if (eol == NULL)
			eol = bol + strlen(bol);

		line_len = eol - bol;

		/* This line matches the prefix. Copy into meta */
		if (line_len >= gcli_pref_len &&
		    strncmp(bol, gcli_pref, gcli_pref_len) == 0) {
			strncat(meta,
			        bol + gcli_pref_len,
			        line_len - gcli_pref_len + 1);
		} else {
			strncat(comment, bol, line_len + 1);
		}

		bol = eol + 1;
		if (*bol == '\0')
			break;
	}

	details->gcli_meta = meta;
	details->body = comment;
}

static void
extract_patch_comments(struct review_ctx *ctx, struct gcli_diff_comments *out)
{
	FILE *f = fopen(ctx->diff_path, "r");
	struct gcli_diff_parser p = {0};
	struct gcli_patch_series series = {0};

	TAILQ_INIT(&series.patches);

	if (gcli_diff_parser_from_file(f, ctx->diff_path, &p) < 0)
		err(1, "error: failed to open diff");

	if (gcli_parse_patch_series(&p, &series) < 0)
		errx(1, "error: failed to parse patch");

	if (gcli_patch_series_get_comments(&series, out) < 0)
		errx(1, "error: failed to get comments");

	process_series_prelude(series.prelude, &ctx->details);

	gcli_free_patch_series(&series);
	gcli_free_diff_parser(&p);
	fclose(f);
}

static void
edit_diff(struct review_ctx *ctx)
{
	if (access(ctx->diff_path, F_OK) < 0) {
		fetch_patch(ctx);
	} else {
		/* The file exists, ask whether to open again or to delete and start over. */
		if (sn_yesno("There seems to already be a review in progress. Start over?"))
			fetch_patch(ctx);
	}

	gcli_editor_open_file(g_clictx, ctx->diff_path);
	extract_patch_comments(ctx, &ctx->details.comments);

	free(ctx->diff_path);
}

static void
print_comment_list(struct gcli_diff_comments const *comments)
{
	struct gcli_diff_comment const *comment;

	TAILQ_FOREACH(comment, comments, next) {
		printf("%s: %d: %s\n%s\n\n",
		       comment->after.filename, comment->after.start_row,
		       comment->comment,
		       comment->diff_text);
	}
}

static int
ask_for_review_state(void)
{
	int state = 0;

	do {
		int c;

		printf("What do you want to do with the review? [Leave a (C)omment, (R)equest changes, (A)ccept, (P)ostpone] ");
		fflush(stdout);

		c = getchar();
		switch (c) {
		case EOF:
			fprintf(stderr, "\nAborted\n");
			exit(1);
		case 'a': case 'A':
			state = GCLI_REVIEW_ACCEPT_CHANGES;
			break;
		case 'r': case 'R':
			state = GCLI_REVIEW_REQUEST_CHANGES;
			break;
		case 'c': case 'C':
			state = GCLI_REVIEW_COMMENT;
			break;
		case 'p': case 'P':
			state = -1;
			break;
		default:
			fprintf(stderr, "error: unrecognised answer\n");
			break;
		}
	} while (!state);

	return state;
}

static void
do_review_session(char const *owner, char const *repo, gcli_id const pull_id)
{
	struct review_ctx ctx = {
		.details = {
			.owner = owner,
			.repo = repo,
			.pull_id = pull_id,
		},
		.diff_path = get_review_diff_file_name(owner, repo, pull_id),
	};

	edit_diff(&ctx);
	printf("\nThese are your comments:\n");
	print_comment_list(&ctx.details.comments);

	if (ctx.details.review_state == 0)
		ctx.details.review_state = ask_for_review_state();

	if (ctx.details.review_state < 0) {
		printf("Review has been postponed. You can pick up again by rerunning the review subcommand.\n");
		return;
	}

	if (gcli_pull_create_review(g_clictx, &ctx.details) < 0)
		errx(1, "error: failed to create review: %s", gcli_get_error(g_clictx));
}

int
subcommand_pull_review(int argc, char *argv[])
{
	int ch;
	char const *owner = NULL, *repo = NULL;
	gcli_id pull_id;
	bool have_id = false;

	struct option options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o', },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r', },
		{ .name = "id",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'i', },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "+o:r:i:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			owner = optarg;
		} break;
		case 'r': {
			repo = optarg;
		} break;
		case 'i': {
			char *endptr;
			pull_id = strtoul(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "error: bad PR id %s\n", optarg);

			have_id = true;
		} break;
		default: {
			usage();
			return EXIT_FAILURE;
		} break;
		}
	}

	argc -= optind;
	argv += optind;

	if (!have_id) {
		fprintf(stderr, "error: missing PR id. use -i <id>.\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);
	do_review_session(owner, repo, pull_id);

	return EXIT_SUCCESS;
}
