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

 /* Parser helpers for Bugzilla */

#include <config.h>

#include <gcli/bugzilla/bugs-parser.h>
#include <gcli/json_util.h>

#include <templates/bugzilla/bugs.h>

int
parse_bugzilla_comments_array_skip_first(struct  gcli_ctx *ctx,
                                         struct json_stream *stream,
                                         struct gcli_comment_list *out)
{
	int rc = 0;

	if (json_next(stream) != JSON_ARRAY)
		return gcli_error(ctx, "expected array for comments array");

	SKIP_OBJECT_VALUE(stream);

	while (json_peek(stream) != JSON_ARRAY_END) {
		out->comments = realloc(out->comments, sizeof(*out->comments) * (out->comments_size + 1));
		memset(&out->comments[out->comments_size], 0, sizeof(out->comments[out->comments_size]));
		rc = parse_bugzilla_comment(ctx, stream, &out->comments[out->comments_size++]);
		if (rc < 0)
			return rc;
	}

	if (json_next(stream) != JSON_ARRAY_END)
		return gcli_error(ctx, "unexpected element in array while parsing");

	return 0;
}

int
parse_bugzilla_comments_array_only_first(struct gcli_ctx *ctx,
                                         struct json_stream *stream, char **out)
{
	int rc = 0;

	if (json_next(stream) != JSON_ARRAY)
		return gcli_error(ctx, "expected array for comments array");

	rc = parse_bugzilla_comment_text(ctx, stream, out);
	if (rc < 0)
		return rc;

	while (json_peek(stream) != JSON_ARRAY_END) {
		SKIP_OBJECT_VALUE(stream);
	}

	if (json_next(stream) != JSON_ARRAY_END)
		return gcli_error(ctx, "unexpected element in array while parsing");

	return 0;
}

int
parse_bugzilla_bug_comments_dictionary_skip_first(struct gcli_ctx *const ctx,
                                                  struct json_stream *stream,
                                                  struct gcli_comment_list *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla comments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_comments_internal_skip_first(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla comments dictionary");

	return rc;
}

int
parse_bugzilla_bug_comments_dictionary_only_first(struct gcli_ctx *const ctx,
                                                  struct json_stream *stream,
                                                  char **out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla comments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_comments_internal_only_first(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla comments dictionary");

	return rc;
}

int
parse_bugzilla_assignee(struct gcli_ctx *ctx, struct json_stream *stream,
                        struct gcli_issue *out)
{
	out->assignees = calloc(1, sizeof (*out->assignees));
	out->assignees_size = 1;

	return get_string(ctx, stream, out->assignees);
}

int
parse_bugzilla_bug_attachments_dict(struct gcli_ctx *ctx,
                                    struct json_stream *stream,
                                    struct gcli_attachment_list *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla attachments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_bug_attachments_internal(ctx, stream,
		                                             &out->attachments,
		                                             &out->attachments_size);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla attachments dictionary");

	return rc;
}

int
parse_bugzilla_attachment_content_only_first(struct gcli_ctx *ctx,
                                             struct json_stream *stream,
                                             struct gcli_attachment *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla attachments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_bug_attachment(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla attachments dictionary");

	return rc;
}

int
parse_bugzilla_single_comments_array_only_first(struct gcli_ctx *ctx,
                                                struct json_stream *stream,
                                                struct gcli_comment *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla comments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_comment(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla comments dictionary");

	return rc;
}
