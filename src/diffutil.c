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
gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out)
{
	if (gcli_diff_parse_prelude(parser, out) < 0)
		return -1;

	TAILQ_INIT(&out->hunks);

	/* TODO cleanup */
	while (parser->hd[0] == 'd') {
		gcli_diff_hunk *h = calloc(sizeof(*h), 1);
		if (gcli_diff_parse_hunk(parser, h) < 0)
			return -1;

		TAILQ_INSERT_TAIL(&out->hunks, h, next);
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
gcli_diff_parse_prelude(gcli_diff_parser *parser, gcli_diff *out)
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
parse_hunk_range_info(gcli_diff_parser *parser, gcli_diff_chunk *out)
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

	return 0;
}

static int
parse_hunk_diff_line(gcli_diff_parser *parser, gcli_diff_hunk *out)
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
parse_hunk_index_line(gcli_diff_parser *parser, gcli_diff_hunk *out)
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
read_chunk_body(gcli_diff_parser *parser, gcli_diff_chunk *chunk)
{
	struct token buf = {0};
	size_t buf_len;

	buf.start = parser->hd;
	buf.end = parser->hd;

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

		buf.end = line.end + 1;
		parser->hd = line.end + 1;
	}

	buf_len = token_len(&buf);
	chunk->body = calloc(buf_len + 1, 1);
	strncpy(chunk->body, buf.start, buf_len);

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
try_parse_new_file_mode(gcli_diff_parser *parser,
                        gcli_diff_hunk *out)
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
gcli_diff_parse_hunk(gcli_diff_parser *parser, gcli_diff_hunk *out)
{
	if (parse_hunk_diff_line(parser, out) < 0)
		return -1;

	if (try_parse_new_file_mode(parser, out) < 0)
		return -1;

	if (parse_hunk_index_line(parser, out) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'a', &out->r_file) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'b', &out->a_file) < 0)
		return -1;

	TAILQ_INIT(&out->chunks);
	while (parser->hd[0] == '@') {
		gcli_diff_chunk *chunk = calloc(sizeof(*chunk), 1);
		if (parse_hunk_range_info(parser, chunk) < 0) {
			free(chunk);
			return -1;
		}

		if (read_chunk_body(parser, chunk) < 0) {
			free(chunk);
			return -1;
		}

		TAILQ_INSERT_TAIL(&out->chunks, chunk, next);
	}

	return 0;
}

void
gcli_free_diff_chunk(gcli_diff_chunk *chunk)
{
	free(chunk->context_info);
	chunk->context_info = NULL;

	free(chunk->body);
	chunk->body = NULL;
}

void
gcli_free_diff_hunk(gcli_diff_hunk *hunk)
{
	gcli_diff_chunk *d;

	free(hunk->file_a);
	hunk->file_a = NULL;

	free(hunk->file_b);
	hunk->file_b = NULL;

	free(hunk->hash_a);
	hunk->hash_a = NULL;

	free(hunk->hash_b);
	hunk->hash_b = NULL;

	free(hunk->file_mode);
	hunk->file_mode = NULL;

	free(hunk->r_file);
	hunk->r_file = NULL;

	free(hunk->a_file);
	hunk->a_file = NULL;

	d = TAILQ_FIRST(&hunk->chunks);
	while (d) {
		gcli_diff_chunk *n = TAILQ_NEXT(d, next);
		gcli_free_diff_chunk(d);
		free(d);
		d = n;
	}
	TAILQ_INIT(&hunk->chunks);
}

void
gcli_free_diff(gcli_diff *diff)
{
	gcli_diff_hunk *h, *n;
	free(diff->prelude);
	diff->prelude = NULL;

	h = TAILQ_FIRST(&diff->hunks);
	while (h) {
		n = TAILQ_NEXT(h, next);
		gcli_free_diff_hunk(h);
		free(h);
		h = n;
	}
	TAILQ_INIT(&diff->hunks);
}

void
gcli_free_diff_parser(gcli_diff_parser *parser)
{
	if (parser->buf_needs_free)
		free((char *)parser->buf);

	memset(parser, 0, sizeof(*parser));
}
