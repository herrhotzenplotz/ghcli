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

#include <assert.h>
#include <string.h>

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

	if (parser->hd[0] != '\0')
		return -1;

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
		if (read_number(&line, 10, &out->range_r_end) < 0)
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
		if (read_number(&line, 10, &out->range_a_end) < 0)
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
struct comment_read_ctx {
	gcli_diff const *diff;
	gcli_diff_hunk const *hunk;
	gcli_diff_comments *comments;
	char const *front;
	int patched_line_offset;       /* Offset of the comment in the final patched file */
	int diff_line_offset;          /* Offset of the comment within the current diff */
};

static gcli_diff_comment *
make_comment(struct comment_read_ctx *ctx, char *text,
             int line_number, int diff_line_offset)
{
	gcli_diff_comment *comment = calloc(sizeof(*comment), 1);
	comment->filename = strdup(ctx->diff->file_b);
	comment->comment = text;
	comment->start_row = line_number;
	comment->end_row = line_number;
	comment->diff_line_offset = diff_line_offset;

	return comment;
}

static int
read_comment_unprefixed(struct comment_read_ctx *ctx)
{
	char const *start = ctx->front;
	int const line = ctx->patched_line_offset;
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

	cmt = make_comment(ctx, text, line, diff_line_offset);
	TAILQ_INSERT_TAIL(ctx->comments, cmt, next);

	return 0;
}

static int
read_comment_prefixed(struct comment_read_ctx *ctx)
{
	char const *start = ctx->front;
	int const line = ctx->patched_line_offset;
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

	cmt = make_comment(ctx, text, line, diff_line_offset);
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
		.patched_line_offset = hunk->range_a_start,
		.diff_line_offset = hunk->diff_line_offset,
	};
	int diff_range_off = -1;

	for (;;) {
		switch (*ctx.front) {
		case '\0':
			break;
		case ' ':
		case '+':
			ctx.patched_line_offset += 1;
		case '-':
			ctx.diff_line_offset += 1;
			if (diff_range_off >= 0)
				diff_range_off += 1;
			break;
		case '{':
			diff_range_off = 0;
			break;
		case '}': {
			if (diff_range_off < 0)
				return -1;             /* not in a range */

			gcli_diff_comment *c = TAILQ_LAST(out, gcli_diff_comments);
			if (!c)
				return -1;             /* no comment to refer to */

			c->end_row = c->start_row + diff_range_off - 1;
			diff_range_off = -1;
		} break;
		default: {
			/* comment */
			if (read_comment(&ctx) < 0)
				return -1;

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

	TAILQ_INIT(out);

	TAILQ_FOREACH(diff, &patch->diffs, next) {
		if (gcli_diff_get_comments(diff, out) < 0)
			return -1;
	}

	return 0;
}
