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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/editor.h>
#include <gcli/diffutil.h>
#include <gcli/pulls.h>

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

static int
do_mkdir(char const *const path)
{
	struct stat sb = {0};

	if (stat(path, &sb) < 0)
		return mkdir(path, 0750);

	if (S_ISDIR(sb.st_mode))
		return 0;

	errno = ENOTDIR;
	return -1;
}

static int
mkdir_p(char const *const dirname)
{
	char *d = NULL, *hd = NULL;
	size_t l;

	l = strlen(dirname);
	d = hd = strdup(dirname);
	if (*d == '/')
		d++;

	while (*d) {
		char *p = strchr(d, '/');

		if (!p || !(*p))
			break;

		*p = '\0';
		if (do_mkdir(hd) < 0)
			return -1;

		*p = '/';
		d = p+1;
	}

	if (dirname[l-1] != '/') {
		if (do_mkdir(hd) < 0)
			return -1;
	}

	free(hd);

	return 0;
}

static void
ensure_cache_dir_exists(void)
{
	char *dir = get_review_file_cache_dir();
	if (mkdir_p(dir) < 0)
		err(1, "gcli: error: could not create cache directory");

	free(dir);
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
		err(1, "gcli: error: cannot open %s", ctx->diff_path);

	if (gcli_pull_get_diff(g_clictx, f, ctx->details.owner, ctx->details.repo, ctx->details.pull_id) < 0) {
		errx(1, "gcli: error: failed to get patch: %s",
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
	char *comment, *bol;
	size_t const p_len = strlen(prelude);

	bol = prelude;

	comment = calloc(p_len + 1, 1);

	TAILQ_INIT(&details->meta_lines);

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
			struct gcli_review_meta_line *ml;

			char *meta = calloc(line_len - gcli_pref_len + 1, 1);
			memcpy(meta, bol + gcli_pref_len, line_len - gcli_pref_len);

			ml = calloc(1, sizeof(*ml));
			ml->entry = meta;

			TAILQ_INSERT_TAIL(&details->meta_lines, ml, next);

		} else {
			strncat(comment, bol, line_len + 1);
		}

		bol = eol + 1;
		if (*bol == '\0')
			break;
	}

	details->body = comment;
}

static void
extract_patch_comments(struct review_ctx *ctx, struct gcli_diff_comments *out)
{
	FILE *f = fopen(ctx->diff_path, "r");
	struct gcli_diff_parser p = {0};
	struct gcli_patch patch = {0};

	if (gcli_diff_parser_from_file(f, ctx->diff_path, &p) < 0)
		err(1, "gcli: error: failed to open diff");

	if (gcli_parse_patch(&p, &patch) < 0)
		errx(1, "gcli: error: failed to parse patch");

	if (gcli_patch_get_comments(&patch, out) < 0)
		errx(1, "gcli: error: failed to get comments");

	process_series_prelude(patch.prelude, &ctx->details);

	gcli_free_patch(&patch);
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
gcli_pretty_print_diff(char const *const input)
{
	char const *hd = input;

	for (;;) {
		char const *eol;
		char const *start_colour, *end_colour;
		size_t linelen;

		if (hd == NULL || *hd == '\0')
			return;

		eol = strchr(hd, '\n');
		if (eol == NULL)
			eol = hd + strlen(hd);

		linelen = eol - hd;
		end_colour = gcli_resetcolour();
		if (*hd == '+')
			start_colour = gcli_setcolour(GCLI_COLOR_GREEN);
		else if (*hd == '-')
			start_colour = gcli_setcolour(GCLI_COLOR_RED);
		else
			start_colour = "";

		printf("%s%.*s%s\n", start_colour, (int)linelen, hd, end_colour);
		hd = eol + 1;
	}
}

static void
print_comment_list(struct gcli_diff_comments const *comments)
{
	struct gcli_diff_comment const *comment;

	TAILQ_FOREACH(comment, comments, next) {
		printf("=====================================\n");
		printf("%s:%d:\n", comment->after.filename, comment->after.start_row);
		gcli_pretty_print(comment->comment, 6, 80, stdout);
		printf("The diff is:\n\n");
		gcli_pretty_print_diff(comment->diff_text);
	}

	printf("=====================================\n");
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
			fprintf(stderr, "gcli: error: unrecognised answer\n");
			break;
		}
	} while (!state);

	return state;
}

void
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

	ensure_cache_dir_exists();

	TAILQ_INIT(&ctx.details.comments);

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
		errx(1, "gcli: error: failed to create review: %s", gcli_get_error(g_clictx));
}
