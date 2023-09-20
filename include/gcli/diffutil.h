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

#ifndef GCLI_DIFFUTIL_H
#define GCLI_DIFFUTIL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

typedef struct gcli_diff_hunk gcli_diff_hunk;
struct gcli_diff_hunk {
	TAILQ_ENTRY(gcli_diff_hunk) next;

	int range_a_start, range_a_end, range_r_start, range_r_end;
	char *context_info;
	char *body;

	int diff_line_offset;           /* line offset of this hunk inside the diff */
};

typedef struct gcli_diff gcli_diff;
struct gcli_diff {
	TAILQ_ENTRY(gcli_diff) next;    /* Tailq ntex pointer */

	char *file_a, *file_b;
	char *hash_a, *hash_b;
	char *file_mode;
	int new_file_mode;

	char *r_file, *a_file;   /* file with removals and additions */

	TAILQ_HEAD(, gcli_diff_hunk) hunks;
};

typedef struct gcli_patch gcli_patch;
struct gcli_patch {
	char *prelude;             /* Text leading up to the first diff */

	TAILQ_ENTRY(gcli_patch) next; /* Next pointer in patch series */
	TAILQ_HEAD(, gcli_diff) diffs;
};

typedef struct gcli_patch_series gcli_patch_series;
TAILQ_HEAD(gcli_patch_series, gcli_patch);

typedef struct gcli_diff_parser gcli_diff_parser;
struct gcli_diff_parser {
	char const *buf, *hd;
	size_t buf_size;
	char const *filename;
	int col, row;

	int diff_line_offset;

	/* The parser was initialised with gcli_diff_parser_from_file
	 * which allocates on the heap. We need to free this when
	 * asked to clean up the parser. */
	bool buf_needs_free;
};

/* A single comment referring to a chunk in a dif */
typedef struct gcli_diff_comment gcli_diff_comment;
struct gcli_diff_comment {
	TAILQ_ENTRY(gcli_diff_comment) next;

	char *filename;            /* right side file name */
	int start_row, end_row;    /* right side start- and end row */
	int diff_line_offset;      /* line offset inside the diff */

	char *comment;             /* text of the comment */
	char *diff_text;           /* the diff text this comment refers to */
};

typedef struct gcli_diff_comments gcli_diff_comments;
TAILQ_HEAD(gcli_diff_comments, gcli_diff_comment);

int gcli_diff_parser_from_buffer(char const *buf, size_t buf_size,
                                 char const *filename,
                                 gcli_diff_parser *out);
int gcli_diff_parser_from_file(FILE *f, char const *filename,
                               gcli_diff_parser *out);

int gcli_parse_patch_series(struct gcli_diff_parser *parser,
                            struct gcli_patch_series *out);

int gcli_parse_patch(struct gcli_diff_parser *parser, struct gcli_patch *out);

int gcli_parse_diff(struct gcli_diff_parser *parser, struct gcli_diff *out);

int gcli_parse_patch(gcli_diff_parser *parser, gcli_patch *out);
int gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out);
int gcli_patch_parse_prelude(gcli_diff_parser *parser, gcli_patch *out);
int gcli_patch_get_comments(gcli_patch const *patch, gcli_diff_comments *out);
int gcli_patch_series_get_comments(gcli_patch_series const *series,
                                   gcli_diff_comments *out);
void gcli_free_diff(gcli_diff *diff);
void gcli_free_diff_hunk(gcli_diff_hunk *hunk);
void gcli_free_patch(gcli_patch *patch);

void gcli_free_diff(struct gcli_diff *diff);

void gcli_free_diff_hunk(struct gcli_diff_hunk *hunk);

void gcli_free_diff_parser(gcli_diff_parser *parser);
void gcli_free_patch_series(struct gcli_patch_series *series);

#endif /* GCLI_DIFFUTIL_H */
