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

#ifndef CURL_H
#define CURL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

typedef int (*parsefn)(struct gcli_ctx *, struct json_stream *stream,
                       void *list, size_t *listsize);
typedef void (*filterfn)(void *list, size_t *listsize, void const *userdata);

struct gcli_fetch_buffer {
	char *data;
	size_t length;
};

struct gcli_fetch_list_ctx {
	void *listp;                /* pointer to pointer of start of list */
	size_t *sizep;              /* pointer to list size */
	int max;

	parsefn parse;              /* json parse routine */
	filterfn filter;            /* optional filter */
	void const *userdata;
};

void gcli_fetch_buffer_free(struct gcli_fetch_buffer *buffer);

int gcli_fetch(struct gcli_ctx *ctx, char const *url, char **pagination_next,
               struct gcli_fetch_buffer *out);

int gcli_curl(struct gcli_ctx *ctx, FILE *stream, char const *url,
              char const *content_type);

int gcli_fetch_with_method(struct gcli_ctx *ctx, char const *method,
                           char const *url, char const *data,
                           char **pagination_next,
                           struct gcli_fetch_buffer *out);

int gcli_post_upload(struct gcli_ctx *ctx, char const *url,
                     char const *content_type, void *buffer, size_t buffer_size,
                     struct gcli_fetch_buffer *out);

int gcli_curl_gitea_upload_attachment(struct gcli_ctx *ctx, char const *url,
                                      char const *filename,
                                      struct gcli_fetch_buffer *out);

int gcli_curl_test_success(struct gcli_ctx *ctx, char const *url);
char *gcli_urlencode(char const *);
sn_sv gcli_urlencode_sv(sn_sv const);
char *gcli_urldecode(struct gcli_ctx *ctx, char const *input);
int gcli_fetch_list(struct gcli_ctx *ctx, char *url,
                    struct gcli_fetch_list_ctx *fctx);

#endif /* CURL_H */
