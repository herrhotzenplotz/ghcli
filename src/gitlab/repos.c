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

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <templates/gitlab/repos.h>

#include <assert.h>
#include <stdarg.h>

int
gitlab_get_repo(struct gcli_ctx *ctx, struct gcli_path const *const path,
                struct gcli_repo *const out)
{
	/* GET /projects/:id */
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	int rc;

	rc = gitlab_repo_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_repo(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

static void
gitlab_repos_fixup_missing_visibility(struct gcli_repo_list *const list)
{
	static char const *const public = "public";

	/* Gitlab does not return a visibility field in the repo object on
	 * unauthenticated API requests. We fix up the missing field here
	 * assuming that the repository must be public. */
	for (size_t i = 0; i < list->repos_size; ++i) {
		if (!list->repos[i].visibility)
			list->repos[i].visibility = strdup(public);
	}
}

int
gitlab_get_repos(struct gcli_ctx *ctx, char const *owner, int const max,
                 struct gcli_repo_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &list->repos,
		.sizep = &list->repos_size,
		.parse = (parsefn)(parse_gitlab_repos),
		.max = max,
	};

	e_owner = gcli_urlencode(owner);
	url = sn_asprintf("%s/users/%s/projects", gcli_get_apibase(ctx), e_owner);
	free(e_owner);

	rc = gcli_fetch_list(ctx, url, &fl);

	if (rc == 0)
		gitlab_repos_fixup_missing_visibility(list);

	return rc;
}

int
gitlab_repo_delete(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_repo_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_repo_create(struct gcli_ctx *ctx, struct gcli_repo_create_options const *options,
                   struct gcli_repo *out)
{
	char *url, *payload;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	int rc;
	struct json_stream stream = {0};

	/* Request preparation */
	url = sn_asprintf("%s/projects", gcli_get_apibase(ctx));

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "name");
		gcli_jsongen_string(&gen, options->name);

		gcli_jsongen_objmember(&gen, "description");
		gcli_jsongen_string(&gen, options->description);

		gcli_jsongen_objmember(&gen, "visibility");
		gcli_jsongen_string(&gen, options->private ? "private" : "public");
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Fetch and parse result */
	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, out ? &buffer : NULL);
	if (rc == 0 && out) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_repo(ctx, &stream, out);

		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(payload);
	free(url);

	return rc;
}

int
gitlab_repo_set_visibility(struct gcli_ctx *ctx,
                           struct gcli_path const *const path,
                           gcli_repo_visibility vis)
{
	char *url;
	char const *vis_str;
	char *payload;
	int rc;

	switch (vis) {
	case GCLI_REPO_VISIBILITY_PRIVATE:
		vis_str = "private";
		break;
	case GCLI_REPO_VISIBILITY_PUBLIC:
		vis_str = "public";
		break;
	default:
		assert(false && "Invalid visibility");
		return gcli_error(ctx, "bad visibility level");
	}

	rc = gitlab_repo_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	payload = sn_asprintf("{ \"visibility\": \"%s\" }", vis_str);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}

int
gitlab_repo_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     char **url, char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = sn_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo = NULL;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/projects/%s%%2F%s%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab repos");
	} break;
	}

	free(suffix);

	return rc;
}
