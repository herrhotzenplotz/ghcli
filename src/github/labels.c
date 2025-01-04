/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/github/labels.h>
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/labels.h>

int
github_get_labels(struct gcli_ctx *ctx, struct gcli_path const *const path,
                  int const max, struct gcli_label_list *const out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->labels,
		.sizep= &out->labels_size,
		.parse = (parsefn)(parse_github_labels),
		.max = max,
	};

	*out = (struct gcli_label_list) {0};

	rc = github_repo_make_url(ctx, path, &url, "/labels");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_create_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_label *const label)
{
	char *url = NULL, *payload = NULL, *colour = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	struct json_stream stream = {0};

	/* Generate URL: /repos/{owner}/{repo}/labels */
	rc = github_repo_make_url(ctx, path, &url, "/labels");
	if (rc < 0)
		return rc;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "name");
		gcli_jsongen_string(&gen, label->name);

		gcli_jsongen_objmember(&gen, "description");
		gcli_jsongen_string(&gen, label->description);

		colour = sn_asprintf("%06X", label->colour & 0xFFFFFF);

		gcli_jsongen_objmember(&gen, "color");
		gcli_jsongen_string(&gen, colour);

		free(colour);
		colour = NULL;
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_label(ctx, &stream, label);
		json_close(&stream);
	}

	free(url);
	free(payload);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_delete_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    char const *label)
{
	char *url = NULL;
	char *e_label = NULL;
	int rc = 0;

	e_label = gcli_urlencode(label);

	/* DELETE /repos/{owner}/{repo}/labels/{name} */
	rc = github_repo_make_url(ctx, path, &url, "/lables/%s", e_label);

	if (rc == 0)
		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_label);

	return rc;
}
