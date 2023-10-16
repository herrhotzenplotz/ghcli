/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/diffutil.h>

#include <sn/sn.h>

#include <assert.h>
#include <string.h>

struct token;
static bool is_patch_separator(struct token const *line);

int
gcli_diff_parser_from_buffer(char const *buf, size_t buf_size,
                             char const *filename, gcli_diff_parser *out)
{
	out->buf = out->hd = buf;
	out->buf_size = buf_size;
	out->filename = filename;
	out->col = out->row = 1;

	return 0;
}

int
gcli_diff_parser_from_file(FILE *f, char const *filename,
                           gcli_diff_parser *out)
{
	long len = 0;
	char *buf;

	if (fseek(f, 0, SEEK_END) < 0)
		return -1;

	if ((len = ftell(f)) < 0)
		return -1;

	if (fseek(f, 0, SEEK_SET) < 0)
		return -1;

	buf = malloc(len + 1);
	buf[len] = '\0';
	fread(buf, len, 1, f);

	out->buf_needs_free = true;

	return gcli_diff_parser_from_buffer(buf, len, filename, out);
}

int
gcli_parse_patch(gcli_diff_parser *parser, gcli_patch *out)
{
	if (gcli_patch_parse_prelude(parser, out) < 0)
		return -1;

	TAILQ_INIT(&out->diffs);

	/* TODO cleanup */
	while (parser->hd[0] == 'd') {
		gcli_diff *d = calloc(sizeof(*d), 1);

		if (gcli_parse_diff(parser, d) < 0)
			return -1;

		TAILQ_INSERT_TAIL(&out->diffs, d, next);
	}

	return 0;
}

struct token {
	char const *start;
	char const *end;
};

static inline int
token_len(struct token const *const t)
{
	return t->end - t->start;
}

static int
nextline(gcli_diff_parser *parser, struct token *out)
{
	out->start = parser->hd;
	if (*out->start == '\0')
		return -1;

	out->end = strchr(out->start, '\n');
	if (out->end == NULL)
		out->end = parser->buf + parser->buf_size - 1;

	return 0;
}

static int
read_commit_hash_from_separator(struct token const *line, struct gcli_patch *out)
{
	char const *end_of_hash, *start_of_hash;

	start_of_hash = strchr(line->start, ' ');
	if (!start_of_hash)
		return -1;

	start_of_hash += 1;
	end_of_hash = strchr(start_of_hash, ' ');

	if (!end_of_hash)
		return -1;

	out->commit_hash = sn_strndup(start_of_hash, end_of_hash - start_of_hash);
	return 0;
}

int
gcli_patch_parse_prelude(gcli_diff_parser *parser, gcli_patch *out)
{
	assert(out->prelude == NULL);
	char const *prelude_begin = parser->hd;

	for (;;) {
		struct token line = {0};
		if (nextline(parser, &line) < 0)
			break;

		size_t const line_len = token_len(&line);

		if (line_len > 5 && strncmp(line.start, "diff ", 5) == 0)
			break;

		if (!out->commit_hash) {
			if (is_patch_separator(&line)) {
				int rc;

				rc = read_commit_hash_from_separator(&line, out);
				if (rc < 0)
					return rc;
			}
		}

		parser->hd = line.end + 1;
		parser->col = 1;
		parser->row += 1;
	}

	size_t const prelude_len = parser->hd - prelude_begin;
	out->prelude = calloc(prelude_len + 1, 1);
	memcpy(out->prelude, prelude_begin, prelude_len);

	return 0;
}

static int
readfilename(struct token *line, char **filename)
{
	struct token fname = {0};
	fname.start = fname.end = line->start;
	int escapes = 0;
	char *outbuf;

	for (;;) {
		fname.end = strchr(fname.end, ' ');
		if (fname.end > line->end) {
			fname.end = line->end;
			break;
		}

		if (!fname.end || *(fname.end - 1) != '\\')
			break;

		fname.end += 1; /* skip over space */
		escapes++;
	}

	if (fname.end == NULL)
		fname.end = line->end;

	outbuf = *filename = calloc(token_len(&fname) - escapes + 1, 1);
	for (;;) {
		char const *chunk_end = strchr(fname.start, ' ');
		if (chunk_end > fname.end)
			chunk_end = fname.end;

		strncat(outbuf, fname.start, chunk_end - fname.start);
		fname.start = chunk_end + 1;

		if (fname.start >= fname.end)
			break;
	}

	line->start = fname.start;

	return 0;
}

static int
read_number(struct token *t, int base, int *out)
{
	char *endptr = NULL;

	*out = strtol(t->start, &endptr, base);
	if (endptr == t->start)
		return -1;

	t->start = endptr;
	return 0;
}

static int
expect_prefix(struct token *t, char const *const prefix)
{
	size_t const prefix_len = strlen(prefix);

	if (token_len(t) < (int)prefix_len)
		return -1;

	if (strncmp(t->start, prefix, prefix_len))
		return -1;

	t->start += prefix_len;

	return 0;
}

/* Git uses a patch separator in the format-patch code that ALWAYS
 * looks something like:
 *
 *    "From long-ass-commit-hash-here Mon Sep 17 00:00:00 2001\n"
 *
 * We will 'abuse' this fact here. In fact git itself uses this to
 * separate commits in an e-mailed patch series. */
static bool
is_patch_separator(struct token const *line)
{
	size_t const line_len = token_len(line);
	char const prefix[] = "From ";
	char const suffix[] = " Mon Sep 17 00:00:00 2001\n";

	if (line_len < sizeof(prefix) - 1)
		return false;

	if (line_len < sizeof(suffix) - 1)
		return false;

	bool const prefix_matches =
		memcmp(line->start, prefix, sizeof(prefix) - 1) == 0;

	if (!prefix_matches)
		return false;

	/* The commit hash is always 40 chars wide - but only when
	 * using SHA1 commit hashes (version 0 format).
         *
	 * When using version 1 object format, hashes are SHA256
	 * and thus 64 characters long. */
	bool const suffix_matches =
		memcmp(line->start + (line_len - (sizeof(suffix) - 2)),
		       suffix,
		       sizeof(suffix) - 1) == 0;

	return suffix_matches;
}

static int
parse_hunk_range_info(gcli_diff_parser *parser, gcli_diff_hunk *out)
{
	struct token line = {0};

	if (parser->hd[0] != '@')
		return -1;

	if (nextline(parser, &line) < 0)
		return -1;

	if (expect_prefix(&line, "@@ -") < 0)
		return -1;

	if (read_number(&line, 10, &out->range_r_start) < 0)
		return -1;

	char const delim_r = *line.start++;
	if (delim_r == ',') {
		if (read_number(&line, 10, &out->range_r_length) < 0)
			return -1;
	} else if (delim_r != ' ') {
		return -1;
	}

	if (expect_prefix(&line, " +") < 0)
		return -1;

	if (read_number(&line, 10, &out->range_a_start) < 0)
		return -1;

	char const delim_a = *line.start;
	if (delim_a == ',') {
		line.start += 1;
		if (read_number(&line, 10, &out->range_a_length) < 0)
			return -1;
	} else if (delim_a != ' ') {
		return -1;
	}

	if (expect_prefix(&line, " @@") < 0)
		return -1;

	/* in case of range context info there must be a space */
	if (token_len(&line)) {
		if (*line.start++ != ' ')
			return -1;
	}

	out->context_info = calloc(token_len(&line) + 1, 1);
	strncat(out->context_info, line.start, token_len(&line));

	parser->hd = line.end + 1;
	parser->diff_line_offset += 1;

	return 0;
}

static int
parse_diff_header(gcli_diff_parser *parser, gcli_diff *out)
{
	char const hunk_marker[] = "diff --git ";
	struct token line = {0};

	if (nextline(parser, &line) < 0)
		return -1;

	size_t const line_len = token_len(&line);

	if (line_len < 5)
		return -1;

	if (strncmp(line.start, hunk_marker, sizeof(hunk_marker) - 1))
		return -1;

	line.start += sizeof(hunk_marker) - 1;

	if (line.start[0] != 'a' || line.start[1] != '/')
		return -1;

	/* @@@ bounds check! */
	line.start += 2;
	if (readfilename(&line, &out->file_a) < 0)
		return -1;

	if (line.start[0] != 'b' || line.start[1] != '/')
		return -1;

	line.start += 2;
	if (readfilename(&line, &out->file_b) < 0)
		return -1;

	if (line.start < line.end)
		return -1;

	parser->hd = line.end;
	if (*parser->hd++ != '\n')
		return -1;

	return 0;
}

static int
parse_diff_index_line(gcli_diff_parser *parser, gcli_diff *out)
{
	struct token line = {0};
	char const *tl;

	if (nextline(parser, &line) < 0)
		return -1;

	if (expect_prefix(&line, "index ") < 0)
		return -1;

	tl = strchr(line.start, '.');
	if (tl == NULL)
		return -1;

	out->hash_a = calloc(tl - line.start + 1, 1);
	memcpy(out->hash_a, line.start, tl - line.start);

	line.start = tl;
	if (line.start[0] != '.' || line.start[1] != '.')
		return -1;

	line.start += 2;

	tl = strchr(line.start, ' ');
	if (tl == NULL)
		return -1;

	if (tl > line.end)
		tl = line.end;

	out->hash_b = calloc(tl - line.start + 1, 1);
	memcpy(out->hash_b, line.start, tl - line.start);

	line.start = tl;
	if (line.start[0] == ' ') {
		line.start += 1;

		size_t const fmode_len = token_len(&line);
		out->file_mode = calloc(fmode_len + 1, 1);
		memcpy(out->file_mode, line.start, fmode_len);

		parser->hd = line.start + fmode_len;
		if (*parser->hd++ != '\n')
			return -1;
	} else {
		if (line.start[0] != '\n')
			return -1;

		parser->hd = line.start + 1;
	}

	return 0;
}

static int
get_last_line(struct token *buffer, struct token *out)
{
	if (*buffer->end == '\n')
		buffer->end -= 1;

	out->start = buffer->end;
	out->end = buffer->end;

	while (out->start > buffer->start && out->start[0] != '\n')
		out->start -= 1;

	out->start += 1;

	return 0;
}

static void
fixup_hunk_before_next_patch(struct token *buf)
{
	struct token pbuf = *buf;

	while (token_len(&pbuf)) {
		struct token last_line;

		get_last_line(&pbuf, &last_line);
		char const c =  last_line.start[0];

		if (c == ' ' || c == '+' || c == '-' || c == '}')
			break;

		pbuf.end = last_line.start - 1;
	}

	buf->end = pbuf.end;
}

static int
read_hunk_body(gcli_diff_parser *parser, gcli_diff_hunk *hunk)
{
	struct token buf = {0};
	size_t buf_len;

	buf.start = parser->hd;
	buf.end = parser->hd;

	hunk->diff_line_offset = parser->diff_line_offset;

	for (;;) {
		struct token line = {0};

		if (parser->hd[0] == '\0')
			break;

		if (nextline(parser, &line) < 0)
			return -1;

		if (strncmp(line.start, "diff", 4) == 0)
			break;

		if (strncmp(line.start, "@@", 2) == 0)
			break;

		if (line.start[0] == 'F' && is_patch_separator(&line)) {
			/* In this case we found the next patch.
			 * Remove the patch trailer by scanning backwards
			 * through it until we find the last line that
			 * belongs to a comment or a diff hunk */
			fixup_hunk_before_next_patch(&buf);
			break;
		}

		/* If it is a comment, don't count this line into
		 * the absolute diff offset of the hunk */
		if (line.start[0] == ' ' || line.start[0] == '+' ||
		    line.start[0] == '-')
		{
			parser->diff_line_offset += 1;
		}

		buf.end = line.end + 1;
		parser->hd = line.end + 1;
	}

	buf_len = token_len(&buf);
	hunk->body = calloc(buf_len + 1, 1);
	strncpy(hunk->body, buf.start, buf_len);

	return 0;
}

/* Parse the additions- or removals file name */
static int
parse_hunk_a_or_r_file(gcli_diff_parser *parser, char c, char **out)
{
	struct token line = {0};
	size_t linelen;

	if (nextline(parser, &line) < 0)
		return -1;

	if (expect_prefix(&line, "--- ") == 0) {
		if (token_len(&line) >= 2 && line.start[0] == c &&
		    line.start[1] == '/') {
			line.start += 2;
		} else if (line.start[0] != '/') {
			return -1;
		}
	} else if (expect_prefix(&line, "+++ ") == 0) {
		if (token_len(&line) >= 2 && line.start[0] != c &&
		    line.start[1] != '/') {
			return -1;
		}

		line.start += 2;
	} else {
		return -1;
	}

	linelen = token_len(&line);
	*out = calloc(linelen + 1, 1);
	memcpy(*out, line.start, linelen);

	parser->hd = line.end + 1;

	return 0;
}

static int
try_parse_new_file_mode(gcli_diff_parser *parser, gcli_diff *out)
{
	struct token line = {0};
	char const fmode_prefix[] = "new file mode ";

	if (nextline(parser, &line) < 0)
		return -1;

	/* Don't fail this, might be the index line */
	if (expect_prefix(&line, fmode_prefix) < 0)
		return 0;

	if (read_number(&line, 8, &out->new_file_mode) < 0)
		return -1;

	if (token_len(&line))
		return -1;

	parser->hd = line.end + 1;

	return 0;
}

int
gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out)
{
	if (parse_diff_header(parser, out) < 0)
		return -1;

	if (try_parse_new_file_mode(parser, out) < 0)
		return -1;

	if (parse_diff_index_line(parser, out) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'a', &out->r_file) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'b', &out->a_file) < 0)
		return -1;

	parser->diff_line_offset = 0;
	TAILQ_INIT(&out->hunks);
	while (parser->hd[0] == '@') {
		gcli_diff_hunk *hunk = calloc(sizeof(*hunk), 1);

		if (parse_hunk_range_info(parser, hunk) < 0) {
			free(hunk);
			return -1;
		}

		if (read_hunk_body(parser, hunk) < 0) {
			free(hunk);
			return -1;
		}

		TAILQ_INSERT_TAIL(&out->hunks, hunk, next);
	}

	return 0;
}

static int
patch_series_read_prelude(struct gcli_diff_parser *parser,
                          struct gcli_patch_series *series)
{
	char const *prelude_begin = parser->hd;

	for (;;) {
		struct token line = {0};
		if (nextline(parser, &line) < 0)
			break;

		if (is_patch_separator(&line))
			break;

		parser->hd = line.end + 1;
		parser->col = 1;
		parser->row += 1;
	}

	size_t const prelude_len = parser->hd - prelude_begin;
	series->prelude = calloc(prelude_len + 1, 1);
	memcpy(series->prelude, prelude_begin, prelude_len);

	return 0;
}

int
gcli_parse_patch_series(struct gcli_diff_parser *parser,
                        struct gcli_patch_series *series)
{
	TAILQ_INIT(&series->patches);

	if (patch_series_read_prelude(parser, series) < 0)
		return -1;

	while (parser->hd[0] != '\0') {
		struct gcli_patch *p = calloc(sizeof(*p), 1);

		TAILQ_INSERT_TAIL(&series->patches, p, next);
		if (gcli_parse_patch(parser, p) < 0)
			return -1;
	}

	return 0;
}

void
gcli_free_diff_hunk(gcli_diff_hunk *hunk)
{
	free(hunk->context_info);
	hunk->context_info = NULL;

	free(hunk->body);
	hunk->body = NULL;
}

void
gcli_free_diff(gcli_diff *diff)
{
	free(diff->file_a);
	diff->file_a = NULL;

	free(diff->file_b);
	diff->file_b = NULL;

	free(diff->hash_a);
	diff->hash_a = NULL;

	free(diff->hash_b);
	diff->hash_b = NULL;

	free(diff->file_mode);
	diff->file_mode = NULL;

	free(diff->r_file);
	diff->r_file = NULL;

	free(diff->a_file);
	diff->a_file = NULL;

	gcli_diff_hunk *h = TAILQ_FIRST(&diff->hunks);
	while (h) {
		gcli_diff_hunk *n = TAILQ_NEXT(h, next);
		gcli_free_diff_hunk(h);
		free(h);
		h = n;
	}
	TAILQ_INIT(&diff->hunks);
}

void
gcli_free_patch(gcli_patch *patch)
{
	gcli_diff *d, *n;

	free(patch->prelude);
	patch->prelude = NULL;

	d = TAILQ_FIRST(&patch->diffs);
	while (d) {
		n = TAILQ_NEXT(d, next);
		gcli_free_diff(d);
		free(d);
		d = n;
	}
	TAILQ_INIT(&patch->diffs);
}

void
gcli_free_diff_parser(gcli_diff_parser *parser)
{
	if (parser->buf_needs_free)
		free((char *)parser->buf);

	memset(parser, 0, sizeof(*parser));
}

/********************************************************************
 * Comment Extraction
 *******************************************************************/
struct hunk_line_info {
	int patched_line;
	int original_line;
};

struct comment_read_ctx {
	gcli_diff const *diff;
	gcli_diff_hunk const *hunk;
	gcli_diff_comments *comments;
	char const *front;
	struct hunk_line_info line_info;
	int diff_line_offset;          /* Offset of the comment within the current diff */
	bool last_line_is_new;
};

static gcli_diff_comment *
make_comment(struct comment_read_ctx *ctx, char *text,
             struct hunk_line_info const *line_info, int diff_line_offset)
{
	gcli_diff_comment *comment = calloc(sizeof(*comment), 1);
	comment->after.filename = strdup(ctx->diff->file_b);
	comment->after.start_row = line_info->patched_line;
	comment->after.end_row = line_info->patched_line;
	comment->before.filename = strdup(ctx->diff->file_a);
	comment->before.start_row = line_info->original_line;
	comment->before.end_row = line_info->original_line;
	comment->comment = text;
	comment->diff_line_offset = diff_line_offset;
	comment->commit_hash = strdup(ctx->diff->hash_b);

	return comment;
}

static int
read_comment_unprefixed(struct comment_read_ctx *ctx)
{
	char const *start = ctx->front;
	struct hunk_line_info const line = ctx->line_info;
	int const diff_line_offset = ctx->diff_line_offset;
	size_t comment_len = 0;
	gcli_diff_comment *cmt;
	char *text;

	for (;;) {
		char c = *ctx->front;
		if (c == ' ' || c == '+' || c == '-' || c == '{')
			break;
		else if (c == '\0') /* invalid: comment at end of hunk */
			return -1;

		ctx->front = strchr(ctx->front, '\n');
		if (ctx->front == NULL)
			break;

		ctx->diff_line_offset += 1;
		ctx->front += 1;
	}

	if (ctx->front)
		comment_len = ctx->front - start;
	else
		comment_len = strlen(start);

	text = calloc(comment_len + 1, 1);
	memcpy(text, start, comment_len);

	cmt = make_comment(ctx, text, &line, diff_line_offset);
	TAILQ_INSERT_TAIL(ctx->comments, cmt, next);

	return 0;
}

static int
read_comment_prefixed(struct comment_read_ctx *ctx)
{
	char const *start = ctx->front;
	struct hunk_line_info const line = ctx->line_info;
	int const diff_line_offset = ctx->diff_line_offset;
	size_t comment_len = 0;
	struct gcli_diff_comment *cmt;
	char *text;

	for (;;) {
		char const *c = ctx->front;
		if (*c != '>') {
			if (*c == ' ' || *c == '+' || *c == '-' || *c == '{')
				break;
			else if (*c == '\0') /* invalid: comment at end of hunk */
				return -1;
		}

		ctx->front = strchr(c, '\n');
		if (ctx->front == NULL)
			break;

		/* Skip the prefix chars */
		size_t const prefix_len = c[1] == ' ' ? 2 : 1;
		comment_len += ctx->front - (c + prefix_len) + 1;

		ctx->diff_line_offset += 1;
		ctx->front += 1;
	}

	text = calloc(comment_len + 1, 1);
	while (start < ctx->front) {
		char const *line_end = strchr(start, '\n');
		size_t const prefix_len = start[1] == ' ' ? 2 : 1;
		strncat(text, start + prefix_len, line_end - (start + prefix_len) + 1);
		start = line_end + 1;
	}

	cmt = make_comment(ctx, text, &line, diff_line_offset);
	TAILQ_INSERT_TAIL(ctx->comments, cmt, next);

	return 0;
}

static int
read_comment(struct comment_read_ctx *ctx)
{
	if (strncmp(ctx->front, "> ", 2) == 0)
		return read_comment_prefixed(ctx);
	else
		return read_comment_unprefixed(ctx);
}

static int
gcli_hunk_get_comments(gcli_diff const *diff, gcli_diff_hunk const *hunk,
                       gcli_diff_comments *out)
{
	struct comment_read_ctx ctx = {
		.diff = diff,
		.hunk = hunk,
		.comments = out,
		.front = hunk->body,
		.line_info = {
			.patched_line = hunk->range_a_start,
			.original_line = hunk->range_r_start,
		},
		.diff_line_offset = hunk->diff_line_offset,
		.last_line_is_new = false,
	};
	char const *range_start = NULL;
	bool    in_comment = false,           /* whether we are reading lines referring to a comment */
		in_multiline_comment = false, /* whether the current comment is a multiline comment */
		is_first_line = true;         /* whether this is the first diff line of a comment */

	for (;;) {
		struct gcli_diff_comment *c = TAILQ_LAST(out, gcli_diff_comments);

		char const hd = *ctx.front;

		switch (hd) {
		case '\0':
			break;
		case '+':
		case ' ':
		case '-':
			ctx.diff_line_offset += 1;
			ctx.last_line_is_new = hd == '+';

			if (hd == '+' || hd == ' ')
				ctx.line_info.patched_line += 1;
			if (hd == '-' || hd == ' ')
				ctx.line_info.original_line += 1;

			if (c && !c->diff_text && !in_multiline_comment) {
				char const *end = strchr(ctx.front, '\n');
				if (end == NULL)
					end = ctx.front + strlen(ctx.front);

				end += 1;

				c->diff_text = calloc((end - ctx.front) + 1, 1);
				memcpy(c->diff_text, ctx.front, end - ctx.front);

				c->start_is_in_new = c->end_is_in_new = hd == '+';
			}

			if (c && in_comment && in_multiline_comment) {
				c->end_is_in_new = ctx.last_line_is_new;

				if (is_first_line)
					c->start_is_in_new = ctx.last_line_is_new;

			}
			is_first_line = false;

			break;
		case '{':
			if (!c || c->diff_text)
				return -1;

			range_start = ctx.front;
			in_multiline_comment = true;

			break;
		case '}': {
			assert(range_start != NULL);

			if (!c || !in_comment || !in_multiline_comment)
				return -1;             /* no comment to refer to */

			range_start += 2;

			size_t const len = ctx.front - range_start;
			c->diff_text = calloc(len + 1, 1);
			memcpy(c->diff_text, range_start, len);

			/* FIXME: add deletion-only and addition-only detection here */
			if (c->after.end_row != ctx.line_info.patched_line)
				c->after.end_row = ctx.line_info.patched_line - 1;
			if (c->before.end_row != ctx.line_info.original_line)
				c->before.end_row = ctx.line_info.original_line - 1;

			in_comment = false;
		} break;
		default: {
			/* comment */
			if (read_comment(&ctx) < 0)
				return -1;

			in_comment = true;
			in_multiline_comment = false;
			is_first_line = true;

			continue;
		} break;
		}
		if ((ctx.front = strchr(ctx.front, '\n')) == NULL)
			break;

		ctx.front += 1;
	}

	return 0;
}

static int
gcli_diff_get_comments(gcli_diff const *diff, gcli_diff_comments *out)
{
	gcli_diff_hunk *hunk;

	TAILQ_FOREACH(hunk, &diff->hunks, next) {
		if (gcli_hunk_get_comments(diff, hunk, out) < 0)
			return -1;
	}

	return 0;
}

int
gcli_patch_get_comments(gcli_patch const *patch, gcli_diff_comments *out)
{
	gcli_diff const *diff;

	TAILQ_FOREACH(diff, &patch->diffs, next) {
		if (gcli_diff_get_comments(diff, out) < 0)
			return -1;
	}

	return 0;
}

int
gcli_patch_series_get_comments(struct gcli_patch_series const *series,
                               struct gcli_diff_comments *out)
{
	struct gcli_patch const *patch;

	TAILQ_INIT(out);

	TAILQ_FOREACH(patch, &series->patches, next) {
		if (gcli_patch_get_comments(patch, out) < 0)
			return -1;
	}

	return 0;
}

void
gcli_free_patch_series(struct gcli_patch_series *series)
{
	struct gcli_patch *p = TAILQ_FIRST(&series->patches);
	while (p) {
		struct gcli_patch *n = TAILQ_NEXT(p, next);
		gcli_free_patch(p);
		free(p);
		p = n;
	}
	TAILQ_INIT(&series->patches);
}
