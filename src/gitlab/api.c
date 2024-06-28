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

#include <gcli/gitlab/api.h>
#include <gcli/json_util.h>

#include <templates/gitlab/api.h>

#include <pdjson/pdjson.h>

char const *
gitlab_api_error_string(struct gcli_ctx *ctx, struct gcli_fetch_buffer *const buf)
{
	char *msg = NULL;
	int rc;
	struct json_stream stream = {0};
	struct gitlab_error_data error_data = {0};

	json_open_buffer(&stream, buf->data, buf->length);
	rc = parse_gitlab_get_error(ctx, &stream, &error_data);
	json_close(&stream);

	/* Extract a most useful error message */
	if (error_data.error_description) {
		msg = strdup(error_data.error_description);
	} else if (error_data.message) {
		msg = strdup(error_data.message);
	} else if (error_data.error) {
		msg = strdup(error_data.error);
	} else {
		msg = NULL;
	}

	free(error_data.error_description);
	free(error_data.message);
	free(error_data.error);

	if (rc < 0 || msg == NULL) {
		free(msg);

		if (sn_verbose()) {
			return sn_asprintf("Could not parse Gitlab error response. "
			                   "The response was:\n\n%.*s\n",
			                   (int)buf->length, buf->data);
		} else {
			return strdup("no error message: failed to parse error response. "
			              "Please run the gcli query with verbose mode again.");
		}
	} else {
		return msg;
	}
}

int
gitlab_user_id(struct gcli_ctx *ctx, char const *user_name)
{
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	char *url = NULL;
	char *e_username;
	long uid = -1;
	int rc;

	e_username = gcli_urlencode(user_name);

	url = sn_asprintf("%s/users?username=%s", gcli_get_apibase(ctx),
	                  e_username);

	uid = gcli_fetch(ctx, url, NULL, &buffer);
	if (uid == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		uid = rc = gcli_json_advance(ctx, &stream, "[{s", "id");

		if (rc == 0) {
			rc = get_long(ctx, &stream, &uid);
			json_close(&stream);
		}
	}

	free(e_username);
	free(url);
	gcli_fetch_buffer_free(&buffer);

	return uid;
}
