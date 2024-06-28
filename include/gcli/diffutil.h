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

struct gcli_patch;
struct gcli_patch_series;

struct gcli_diff_hunk {
	TAILQ_ENTRY(gcli_diff_hunk) next;

	int range_a_start, range_a_length, range_r_start, range_r_length;
	char *context_info;
	char *body;

	int diff_line_offset;           /* line offset of this hunk inside the diff */
};

struct gcli_diff {
	TAILQ_ENTRY(gcli_diff) next;    /* Tailq ntex pointer */
	struct gcli_patch *patch;       /* optional: the patch this diff belongs to */

	char *file_a, *file_b;
	char *hash_a, *hash_b;
	char *file_mode;
	int new_file_mode;

	char *r_file, *a_file;   /* file with removals and additions */

	TAILQ_HEAD(gcli_diff_hunks, gcli_diff_hunk) hunks;
};

struct gcli_patch {
	char *prelude;             /* Text leading up to the first diff */
	char *commit_hash;         /* commit hash for this patch */

	struct gcli_patch_series *patch_series;  /* the patch series this patch is a part of */

	TAILQ_ENTRY(gcli_patch) next; /* Next pointer in patch series */
	// FIXME: is this declaration really needed?
	TAILQ_HEAD(gcli_diffs, gcli_diff) diffs;
};

struct gcli_patch_series {
	TAILQ_HEAD(, gcli_patch) patches;

	char *prelude;
};

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
struct gcli_diff_comment {
	TAILQ_ENTRY(gcli_diff_comment) next;

	struct {
		char *filename;
		int start_row, end_row;
	} before, after;           /* range info for before and after applying hunk */

	bool start_is_in_new, end_is_in_new;

	int diff_line_offset;      /* line offset inside the diff */
	char *commit_hash;         /* The commit this comment refers to */
	char *comment;             /* text of the comment */
	char *diff_text;           /* the diff text this comment refers to */
};

TAILQ_HEAD(gcli_diff_comments, gcli_diff_comment);

int gcli_diff_parser_from_buffer(char const *buf, size_t buf_size,
                                 char const *filename,
                                 struct gcli_diff_parser *out);
int gcli_diff_parser_from_file(FILE *f, char const *filename,
                               struct gcli_diff_parser *out);

int gcli_parse_diff(struct gcli_diff_parser *parser, struct gcli_diff *out);

int gcli_parse_patch_series(struct gcli_diff_parser *parser,
                            struct gcli_patch_series *out);

int gcli_parse_patch(struct gcli_diff_parser *parser,
                     struct gcli_patch *out);

int gcli_patch_get_comments(struct gcli_patch const *patch,
                            struct gcli_diff_comments *out);

int gcli_patch_parse_prelude(struct gcli_diff_parser *parser,
                             struct gcli_patch *out);

int gcli_parse_patch(struct gcli_diff_parser *parser, struct gcli_patch *out);

int gcli_parse_diff(struct gcli_diff_parser *parser, struct gcli_diff *out);

int gcli_patch_parse_prelude(struct gcli_diff_parser *parser,
                             struct gcli_patch *out);

int gcli_patch_get_comments(struct gcli_patch const *patch,
                            struct gcli_diff_comments *out);

int gcli_patch_series_get_comments(struct gcli_patch_series const *series,
                                   struct gcli_diff_comments *out);

void gcli_free_diff_parser(struct gcli_diff_parser *parser);

void gcli_free_diff(struct gcli_diff *diff);

void gcli_free_diff_hunk(struct gcli_diff_hunk *hunk);

void gcli_free_patch(struct gcli_patch *patch);

void gcli_free_patch_series(struct gcli_patch_series *series);

void gcli_free_diff(struct gcli_diff *diff);

int gcli_patch_series_get_comments(struct gcli_patch_series const *series,
                                   struct gcli_diff_comments *out);

#endif /* GCLI_DIFFUTIL_H */
