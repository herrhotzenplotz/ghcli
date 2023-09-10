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

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

typedef struct gcli_diff_hunk gcli_diff_hunk;
struct gcli_diff_hunk {
	TAILQ_ENTRY(gcli_diff_hunk) next;    /* Tailq ntex pointer */

	char *file_a, *file_b;
	char *hash_a, *hash_b;
	char *file_mode;

	char *text;             /* Body text */
};

typedef struct gcli_diff gcli_diff;
struct gcli_diff {
	char *prelude;             /* Text leading up to the first diff */

	TAILQ_HEAD(, gcli_diff_hunk) hunks;
};

typedef struct gcli_diff_parser gcli_diff_parser;
struct gcli_diff_parser {
	char *buf, *hd;
	size_t buf_size;
	char const *filename;
	int col, row;
};

int gcli_diff_parser_from_buffer(char *buf, size_t buf_size,
                                 char const *filename,
                                 gcli_diff_parser *out);
int gcli_diff_parser_from_file(FILE *f, char const *filename,
                               gcli_diff_parser *out);
int gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out);
int gcli_diff_parse_hunk(gcli_diff_parser *parser, gcli_diff_hunk *out);
int gcli_diff_parse_prelude(gcli_diff_parser *parser, gcli_diff *out);
void gcli_free_diff(gcli_diff *diff);

#endif /* GCLI_DIFFUTIL_H */
