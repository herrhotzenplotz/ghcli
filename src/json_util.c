/*
 * Copyright 2021-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/date_time.h>
#include <gcli/forges.h>
#include <gcli/json_util.h>
#include <sn/sn.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

int
get_int_(struct gcli_ctx *ctx, json_stream *const input, int *out, char const *where)
{
	if (json_next(input) != JSON_NUMBER)
		return gcli_error(ctx, "unexpected non-integer field in %s", where);

	*out = json_get_number(input);

	return 0;
}

int
get_id_(struct gcli_ctx *ctx, json_stream *const input, gcli_id *out, char const *where)
{
	if (json_next(input) != JSON_NUMBER)
		return gcli_error(ctx, "unexpected non-integer ID field in %s", where);

	*out = json_get_number(input);

	return 0;
}

int
get_long_(struct gcli_ctx *ctx, json_stream *const input, long *out, char const *where)
{
	if (json_next(input) != JSON_NUMBER)
		return gcli_error(ctx, "unexpected non-integer field in %s", where);

	*out = json_get_number(input);

	return 0;
}

int
get_size_t_(struct gcli_ctx *ctx, json_stream *const input, size_t *out, char const *where)
{
	if (json_next(input) != JSON_NUMBER)
		return gcli_error(ctx, "unexpected non-integer field in %s", where);

	*out = json_get_number(input);

	return 0;
}

int
get_double_(struct gcli_ctx *ctx, json_stream *const input, double *out, char const *where)
{
	enum json_type type = json_next(input);

	/* This is dumb but it fixes a couple of weirdnesses of the API */
	if (type == JSON_NULL) {
		*out = 0;
		return 0;
	}

	if (type == JSON_NUMBER) {
		*out = json_get_number(input);
		return 0;
	}

	return gcli_error(ctx, "unexpected non-double field in %s", where);
}

int
get_string_(struct gcli_ctx *ctx, json_stream *const input, char **out,
            char const *where)
{
	enum json_type const type = json_next(input);
	if (type == JSON_NULL) {
		*out = strdup("<empty>");
		return 0;
	}

	if (type != JSON_STRING)
		return gcli_error(ctx, "unexpected non-string field in %s", where);

	size_t len;
	char const *it = json_get_string(input, &len);

	if (!it)
		*out = strdup("<empty>");
	else
		*out = sn_strndup(it, len);

	return 0;
}

int
get_bool_(struct gcli_ctx *ctx,json_stream *const input, bool *out, char const *where)
{
	enum json_type value_type = json_next(input);
	if (value_type == JSON_TRUE) {
		*out = true;
		return 0;
	} else if (value_type == JSON_FALSE || value_type == JSON_NULL) { // HACK
		*out = false;
		return 0;
	}

	return gcli_error(ctx, "unexpected non-boolean value in %s", where);
}

int
get_bool_relaxed_(struct gcli_ctx *ctx,json_stream *const input, bool *out, char const *where)
{
	enum json_type value_type = json_next(input);
	if (value_type == JSON_TRUE) {
		*out = true;
		return 0;
	} else if (value_type == JSON_FALSE || value_type == JSON_NULL) { // HACK
		*out = false;
		return 0;
	} else if (value_type == JSON_NUMBER) {
		*out = json_get_number(input) != 0.0;
		return 0;
	}

	return gcli_error(ctx, "unexpected non-boolean value in %s", where);
}

int
get_user_(struct gcli_ctx *ctx, json_stream *const input, char **out,
          char const *where)
{
	if (json_next(input) != JSON_OBJECT)
		return gcli_error(ctx, "%s: user field is not an object", where);

	char const *expected_key = gcli_forge(ctx)->user_object_key;

	while (json_next(input) == JSON_STRING) {
		size_t len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp(expected_key, key, len) == 0) {
			if (json_next(input) != JSON_STRING)
				return gcli_error(ctx, "%s: login isn't a string", where);

			char const *tmp = json_get_string(input, &len);
			*out = sn_strndup(tmp, len);
		} else {
			json_next(input);
		}
	}

	return 0;
}

static struct {
	char        c;
	char const *with;
} json_escape_table[] = {
	{ .c = '\n', .with = "\\n"  },
	{ .c = '\t', .with = "\\t"  },
	{ .c = '\r', .with = "\\r"  },
	{ .c = '\\', .with = "\\\\" },
	{ .c = '"' , .with = "\\\"" },
};

sn_sv
gcli_json_escape(sn_sv const it)
{
	sn_sv result = {0};

	result.data = calloc(2 * it.length + 1, 1);
	if (!result.data)
		err(1, "malloc");

	for (size_t i = 0; i < it.length; ++i) {
		for (size_t c = 0; c < ARRAY_SIZE(json_escape_table); ++c) {
			if (json_escape_table[c].c == it.data[i]) {
				size_t const len = strlen(json_escape_table[c].with);
				memcpy(result.data + result.length,
				       json_escape_table[c].with,
				       len);
				result.length += len;
				goto next;
			}
		}

		memcpy(result.data + result.length, it.data + i, 1);
		result.length += 1;
	next:
		continue;
	}

	return result;
}

int
get_sv_(struct gcli_ctx *ctx, json_stream *const input, sn_sv *out, char const *where)
{
	enum json_type type = json_next(input);
	if (type == JSON_NULL) {
		*out = SV_NULL;
		return 0;
	}

	if (type != JSON_STRING)
		return gcli_error(ctx, "unexpected non-string field in %s", where);

	size_t len;
	char const *it = json_get_string(input, &len);
	char *copy = sn_strndup(it, len);
	*out = SV(copy);

	return 0;
}

int
get_label_(struct gcli_ctx *ctx, json_stream *const input, char const **out,
           char const *where)
{
	if (json_next(input) != JSON_OBJECT)
		return gcli_error(ctx, "%s: label field is not an object", where);

	while (json_next(input) == JSON_STRING) {
		size_t len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0) {
			if (json_next(input) != JSON_STRING)
				return gcli_error(ctx, "%s: name of the label is not a string",
				                  where);

			*out = json_get_string(input, &len);
			*out = sn_strndup(*out, len);
		} else {
			json_next(input);
		}
	}

	return 0;
}

int
gcli_json_advance(struct gcli_ctx *ctx, json_stream *const stream, char const *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	while (*fmt) {
		switch (*fmt++) {
		case '[': {
			if (json_next(stream) != JSON_ARRAY)
				return gcli_error(ctx, "expected array begin");
		} break;
		case '{': {
			if (json_next(stream) != JSON_OBJECT)
				return gcli_error(ctx, "expected array begin");
		} break;
		case 's': {
			if (json_next(stream) != JSON_STRING)
				return gcli_error(ctx, "expected string");

			char *it = va_arg(ap, char *);
			size_t len = 0;
			char const *other = json_get_string(stream, &len);
			if (strncmp(it, other, len))
				return gcli_error(ctx, "string unmatched");
		} break;
		case ']': {
			if (json_next(stream) != JSON_ARRAY_END)
				return gcli_error(ctx, "expected array end");
		}   break;
		case '}': {
			if (json_next(stream) != JSON_OBJECT_END)
				return gcli_error(ctx, "expected object end");
		}   break;
		case 'i': {
			if (json_next(stream) != JSON_NUMBER)
				return gcli_error(ctx, "expected integer");
		}   break;
		}
	}

	va_end(ap);

	return 0;
}

int
get_parse_int_(struct gcli_ctx *ctx, json_stream *const input, long *out,
               char const *function)
{
	char *endptr = NULL;
	char *string;

	int rc = get_string_(ctx, input, &string, function);
	if (rc < 0)
		return rc;

	*out = strtol(string, &endptr, 10);
	if (endptr != string + strlen(string))
		return gcli_error(ctx, "%s: cannot parse %s as integer", function,
		                  string);

	return 0;
}

int
get_github_style_colour(struct gcli_ctx *ctx, json_stream *const input, uint32_t *out)
{
	char *colour_str;
	char *endptr = NULL;
	int rc;

	rc = get_string(ctx, input, &colour_str);
	if (rc < 0)
		return rc;

	unsigned long colour = strtoul(colour_str, &endptr, 16);
	if (endptr != colour_str + strlen(colour_str))
		return gcli_error(ctx, "%s: bad colour code returned by API",
		                  colour_str);

	free(colour_str);

	*out = ((uint32_t)(colour)) << 8;
	return 0;
}

int
get_gitlab_style_colour(struct gcli_ctx *ctx, json_stream *const input, uint32_t *out)
{
	char *colour;
	char *endptr = NULL;
	long code = 0;
	int rc = 0;

	rc = get_string(ctx, input, &colour);
	if (rc < 0)
		return rc;

	code = strtol(colour + 1, &endptr, 16);
	if (endptr != (colour + 1 + strlen(colour + 1)))
		return gcli_error(ctx, "%s: invalid colour code");

	free(colour);

	*out = ((uint32_t)(code) << 8);

	return 0;
}

int
get_gitea_visibility(struct gcli_ctx *ctx, json_stream *const input, char **out)
{
	bool is_private;
	int rc = get_bool(ctx, input, &is_private);
	if (rc < 0)
		return rc;

	*out = strdup(is_private ? "private" : "public");

	return 0;
}

int
get_gitlab_can_be_merged(struct gcli_ctx *ctx, json_stream *const input, bool *out)
{
	sn_sv tmp;
	int rc = 0;

	rc = get_sv(ctx, input, &tmp);
	if (rc < 0)
		return rc;

	*out = sn_sv_eq_to(tmp, "can_be_merged");
	free(tmp.data);

	return rc;
}

int
get_github_is_pr(struct gcli_ctx *ctx, json_stream *input, int *out)
{
	enum json_type next = json_peek(input);

	(void) ctx;

	if (next == JSON_NULL)
		json_next(input);
	else
		SKIP_OBJECT_VALUE(input);

	*out = (next == JSON_OBJECT);

	return 0;
}

int
get_int_to_sv_(struct gcli_ctx *ctx, json_stream *input, sn_sv *out,
               char const *function)
{
	int rc, val;

	rc = get_int_(ctx, input, &val, function);
	if (rc < 0)
		return rc;

	*out = sn_sv_fmt("%d", val);

	return 0;
}

int
get_github_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                    enum gcli_notification_target_type *out)
{
	sn_sv tmp;
	int rc = 0;

	rc = get_sv(ctx, input, &tmp);
	if (rc < 0)
		return rc;

	if (sn_sv_eq_to(tmp, "Issue")) {
		*out = GCLI_NOTIFICATION_TARGET_ISSUE;

	} else if (sn_sv_eq_to(tmp, "PullRequest")) {
		*out = GCLI_NOTIFICATION_TARGET_PULL_REQUEST;

	} else if (sn_sv_eq_to(tmp, "Release")) {
		*out = GCLI_NOTIFICATION_TARGET_RELEASE;

	} else {
		rc = gcli_error(
			ctx, "bad github notification target type: "SV_FMT,
			SV_ARGS(tmp));

	}

	free(tmp.data);

	return rc;
}

int
get_gitlab_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                    enum gcli_notification_target_type *out)
{
	sn_sv tmp;
	int rc = 0;

	rc = get_sv(ctx, input, &tmp);
	if (rc < 0)
		return rc;

	if (sn_sv_eq_to(tmp, "Issue")) {
		*out = GCLI_NOTIFICATION_TARGET_ISSUE;

	} else if (sn_sv_eq_to(tmp, "MergeRequest")) {
		*out = GCLI_NOTIFICATION_TARGET_PULL_REQUEST;

	} else if (sn_sv_eq_to(tmp, "Commit")) {
		*out = GCLI_NOTIFICATION_TARGET_COMMIT;

	} else if (sn_sv_eq_to(tmp, "Epic")) {
		*out = GCLI_NOTIFICATION_TARGET_EPIC;

	} else {
		rc = gcli_error(
			ctx, "bad github notification target type: "SV_FMT,
			SV_ARGS(tmp));

	}

	free(tmp.data);

	return rc;
}

int
get_gitea_notification_target_type(struct gcli_ctx *ctx, json_stream *input,
                                   enum gcli_notification_target_type *out)
{
	sn_sv tmp;
	int rc = 0;

	rc = get_sv(ctx, input, &tmp);
	if (rc < 0)
		return rc;

	if (sn_sv_eq_to(tmp, "Issue")) {
		*out = GCLI_NOTIFICATION_TARGET_ISSUE;

	} else if (sn_sv_eq_to(tmp, "Pull")) {
		*out = GCLI_NOTIFICATION_TARGET_PULL_REQUEST;

	} else if (sn_sv_eq_to(tmp, "Commit")) {
		*out = GCLI_NOTIFICATION_TARGET_COMMIT;

	} else if (sn_sv_eq_to(tmp, "Repository")) {
		*out = GCLI_NOTIFICATION_TARGET_REPOSITORY;

	} else {
		rc = gcli_error(
			ctx, "bad github notification target type: "SV_FMT,
			SV_ARGS(tmp));

	}

	free(tmp.data);

	return rc;
}

int
get_iso8601_time_(struct gcli_ctx *ctx, json_stream *input, time_t *out,
                  char const *where)
{
	char *copy;
	char const *it;
	enum json_type type;
	size_t len;
	int rc = 0;

	type = json_next(input);
	if (type == JSON_NULL) {
		*out = 0;
		return 0;
	}

	if (type != JSON_STRING)
		return gcli_error(ctx, "unexpected non-string field in %s", where);

	it = json_get_string(input, &len);
	copy = sn_strndup(it, len);

	rc = gcli_parse_iso8601_date_time(ctx, copy, out);

	free(copy);

	return rc;
}

int
get_url_path_(struct gcli_ctx *ctx, struct json_stream *input,
              struct gcli_path *out, char const *function)
{
	char *copy;
	char const *it;
	enum json_type type;
	size_t len;
	int rc = 0;

	type = json_next(input);
	if (type == JSON_NULL) {
		*out = (struct gcli_path){0};
		return 0;
	}

	if (type != JSON_STRING)
		return gcli_error(ctx, "unexpected non-string field in %s", function);

	it = json_get_string(input, &len);
	copy = sn_strndup(it, len);

	out->kind = GCLI_PATH_URL;
	out->data.as_url = copy;

	return rc;
}

int
get_gitlab_notification_target_(struct gcli_ctx *ctx,
                                struct json_stream *input,
                                struct gcli_path *out,
                                char const *where)
{
	if (json_next(input) != JSON_OBJECT)
		return gcli_error(ctx, "%s: notification target is not an object", where);

	out->kind = GCLI_PATH_PID_ID;

	while (json_next(input) == JSON_STRING) {
		size_t len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp("iid", key, len) == 0) {
			if (json_next(input) != JSON_NUMBER) {
				return gcli_error(
					ctx,
					"%s: iid of the target is not a number",
					where);
			}

			out->data.as_pid_id.id = (gcli_id) json_get_number(input);
		} else if (strncmp("project_id", key, len) == 0) {
			if (json_next(input) != JSON_NUMBER) {
				return gcli_error(
					ctx,
					"%s: project_id of the target is not a number",
					where);
			}

			out->data.as_pid_id.project_id = (gcli_id) json_get_number(input);
		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	return 0;
}
