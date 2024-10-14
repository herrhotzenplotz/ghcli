/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GITLAB_SNIPPETS_H
#define GITLAB_SNIPPETS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

struct gcli_gitlab_snippet {
	gcli_id id;
	char *title;
	char *filename;
	char *date;
	char *author;
	char *visibility;
	char *raw_url;
};

struct gcli_gitlab_snippet_list {
	struct gcli_gitlab_snippet *snippets;
	size_t snippets_size;
};

void gcli_snippets_free(struct gcli_gitlab_snippet_list *list);

int gcli_snippets_get(struct gcli_ctx *ctx, int max,
                      struct gcli_gitlab_snippet_list *out);

int gcli_snippet_delete(struct gcli_ctx *ctx, char const *snippet_id);

int gcli_snippet_get(struct gcli_ctx *ctx, char const *snippet_id, FILE *stream);

void gcli_gitlab_snippet_free(struct gcli_gitlab_snippet *snippet);

#endif /* GITLAB_SNIPPETS_H */
