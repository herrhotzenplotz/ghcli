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

#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/curl.h>

#include <pdjson/pdjson.h>

#include <sn/sn.h>

#include <stdint.h>

#include <gcli/status.h>

#define get_int(ctx, input, out)           get_int_(ctx, input, out, __func__)
#define get_id(ctx, input, out)            get_id_(ctx, input, out, __func__)
#define get_long(ctx, input, out)          get_long_(ctx, input, out, __func__)
#define get_size_t(ctx, input, out)        get_size_t_(ctx, input, out, __func__)
#define get_double(ctx, input, out)        get_double_(ctx, input, out, __func__)
#define get_parse_int(ctx, input, out)     get_parse_int_(ctx, input, out, __func__)
#define get_bool(ctx, input, out)          get_bool_(ctx, input, out, __func__)
#define get_bool_relaxed(ctx, input, out)  get_bool_relaxed_(ctx, input, out, __func__)
#define get_string(ctx, input, out)        get_string_(ctx, input, out, __func__)
#define get_sv(ctx, input, out)            get_sv_(ctx, input, out, __func__)
#define get_user(ctx, input, out)          get_user_(ctx, input, out, __func__)
#define get_label(ctx, input, out)         get_label_(ctx, input, out, __func__)
#define get_is_string(ctx, input, out)     ((void)ctx, (*out = json_next(input) == JSON_STRING), 1)
#define get_int_to_string(ctx, input, out) get_int_to_string_(ctx, input, out, __func__)

int get_int_(struct gcli_ctx *ctx, json_stream *input, int *out, char const *function);
int get_id_(struct gcli_ctx *ctx, json_stream *input, gcli_id *out, char const *function);
int get_long_(struct gcli_ctx *ctx, json_stream *input, long *out, char const *function);
int get_size_t_(struct gcli_ctx *ctx, json_stream *input, size_t *out, char const *function);
int get_double_(struct gcli_ctx *ctx, json_stream *input, double *out, char const *function);
int get_parse_int_(struct gcli_ctx *ctx, json_stream *input, long *out, char const *function);
int get_bool_(struct gcli_ctx *ctx, json_stream *input, bool *out, char const *function);
int get_bool_relaxed_(struct gcli_ctx *ctx, json_stream *input, bool *out, char const *function);
int get_string_(struct gcli_ctx *ctx, json_stream *input, char **out, char const *function);
int get_sv_(struct gcli_ctx *ctx, json_stream *input, sn_sv *out, char const *function);
int get_user_(struct gcli_ctx *ctx, json_stream *input, char **out, char const *function);
int get_label_(struct gcli_ctx *ctx, json_stream *input, char const **out, char const *function);
int get_github_style_colour(struct gcli_ctx *ctx, json_stream *input, uint32_t *out);
int get_gitlab_style_colour(struct gcli_ctx *ctx, json_stream *input, uint32_t *out);
int get_github_is_pr(struct gcli_ctx *ctx, json_stream *input, int *out);
int get_github_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                        enum gcli_notification_target_type *out);
int get_gitlab_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                        enum gcli_notification_target_type *out);
int get_gitea_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                       enum gcli_notification_target_type *out);
int get_gitlab_can_be_merged(struct gcli_ctx *ctx, json_stream *input, bool *out);
int get_gitea_visibility(struct gcli_ctx *ctx, json_stream *input, char **out);
sn_sv gcli_json_escape(sn_sv);
#define     gcli_json_escape_cstr(x) (gcli_json_escape(SV((char *)(x))).data)
int gcli_json_advance(struct gcli_ctx *ctx, json_stream *input, char const *fmt, ...);

static inline char const *
gcli_json_bool(bool it)
{
	return it ? "true" : "false";
}

static inline int
get_int_to_string_(struct gcli_ctx *ctx, json_stream *input, char **out,
                   char const *const fn)
{
	int rc;
	long val;

	rc = get_long_(ctx, input, &val, fn);
	if (rc < 0)
		return rc;

	*out = sn_asprintf("%ld", val);

	return 0;
}

#define SKIP_OBJECT_VALUE(stream)	  \
	do { \
		enum json_type value_type = json_next(stream); \
	  \
		switch (value_type) { \
		case JSON_ARRAY: \
			json_skip_until(stream, JSON_ARRAY_END); \
			break; \
		case JSON_OBJECT: \
			json_skip_until(stream, JSON_OBJECT_END); \
			break; \
		default: \
			break; \
		} \
	} while (0)

#endif /* JSON_UTIL_H */
