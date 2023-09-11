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

	return gcli_diff_parser_from_buffer(buf, len, filename, out);
}

int
gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out)
{
	(void) parser;
	(void) out;

	return -1;
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
parse_hunk_range_info(gcli_diff_parser *parser, gcli_diff_chunk *out)
{
	struct token line = {0};

	if (parser->hd[0] != '@')
		return -1;

	if (nextline(parser, &line) < 0)
		return -1;

	if (sscanf(line.start, "@@ -%d,%d +%d,%d @@",
	           &out->range_r_start, &out->range_r_end,
	           &out->range_a_start, &out->range_a_end) != 4)
		return -1;

	line.start = strstr(line.start, " @@");
	if (line.start == NULL)
		return -1;

	/* Skip over the 'space @@', when the context is empty there
	 * is no trailing space */
	line.start += 3;
	if (line.start[0] == ' ')
		line.start += 1;

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
	char index_marker[] = "index ";
	char *tl;

	if (nextline(parser, &line) < 0)
		return -1;

	size_t const line_len = token_len(&line);

	if (line_len < sizeof(index_marker))
		return -1;

	if (strncmp(line.start, index_marker, sizeof(index_marker) - 1))
		return -1;

	line.start += sizeof(index_marker) - 1;

	tl = strchr(line.start, '.');
	if (tl == NULL)
		return -1;

	out->hash_a = calloc(tl - line.start + 1, 1);
	strncat(out->hash_a, line.start, tl - line.start);

	line.start = tl;
	if (line.start[0] != '.' || line.start[1] != '.')
		return -1;

	line.start += 2;

	tl = strchr(line.start, ' ');
	if (tl == NULL)
		return -1;

	out->hash_b = calloc(tl - line.start + 1, 1);
	strncat(out->hash_b, line.start, tl - line.start);

	line.start = tl;
	if (*line.start++ != ' ')
		return -1;

	out->file_mode = calloc(token_len(&line) + 1, 1);
	strncat(out->file_mode, line.start, token_len(&line));

	parser->hd = line.end;
	if (*parser->hd++ != '\n')
		return -1;

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
	char prefix[] = "--- x/";

	switch (c) {
	case 'a':
		strncpy(prefix, "--- a/", sizeof(prefix));
		break;
	case 'b':
		strncpy(prefix, "+++ b/", sizeof(prefix));
		break;
	default:
		assert(0 && "bad diff file name");
	}

	if (nextline(parser, &line) < 0)
		return -1;

	if (token_len(&line) < (int)sizeof(prefix))
		return -1;

	if (strncmp(line.start, prefix, sizeof(prefix) - 1))
		return -1;

	line.start += sizeof(prefix) - 1;
	*out = calloc(token_len(&line) + 1, 1);
	strncat(*out, line.start, token_len(&line));

	parser->hd = line.end + 1;

	return 0;
}

int
gcli_diff_parse_hunk(gcli_diff_parser *parser, gcli_diff_hunk *out)
{
	if (parse_hunk_diff_line(parser, out) < 0)
		return -1;

	if (parse_hunk_index_line(parser, out) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'a', &out->r_file) < 0)
		return -1;

	if (parse_hunk_a_or_r_file(parser, 'b', &out->a_file) < 0)
		return -1;

	TAILQ_INIT(&out->chunks);
	while (parser->hd[0] != '\0') {
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
gcli_free_diff(gcli_diff *diff)
{
	(void) diff;
}
