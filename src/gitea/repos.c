/*
 * Copyright 2022-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitea/repos.h>
#include <gcli/github/repos.h>

#include <gcli/curl.h>

#include <assert.h>

#include <stdarg.h>

int
gitea_get_repos(struct gcli_ctx *ctx, char const *owner, int const max,
                struct gcli_repo_list *const list)
{
	return github_get_repos(ctx, owner, max, list);
}

int
gitea_get_own_repos(struct gcli_ctx *ctx, int const max,
                    struct gcli_repo_list *const list)
{
	return github_get_own_repos(ctx, max, list);
}

int
gitea_repo_create(struct gcli_ctx *ctx, struct gcli_repo_create_options const *options,
                  struct gcli_repo *const out)
{
	return github_repo_create(ctx, options, out);
}

int
gitea_repo_delete(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return github_repo_delete(ctx, path);
}

/* Unlike Github and Gitlab, Gitea only supports private or non-private
 * (thus public) repositories. Separate implementation required. */
int
gitea_repo_set_visibility(struct gcli_ctx *ctx,
                          struct gcli_path const *const path,
                          gcli_repo_visibility vis)
{
	char *url;
	bool is_private;
	char *payload;
	int rc;

	switch (vis) {
	case GCLI_REPO_VISIBILITY_PRIVATE:
		is_private = true;
		break;
	case GCLI_REPO_VISIBILITY_PUBLIC:
		is_private = false;
		break;
	default:
		assert(false && "Invalid visibility");
		return gcli_error(ctx, "bad or unsupported visibility level for Gitea");
	}

	rc = gitea_repo_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	payload = sn_asprintf("{ \"private\": %s }", is_private ? "true" : "false");

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}

int
gitea_repo_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    char **url, char const *const fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, fmt);
	suffix = sn_vasprintf(fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s%s", gcli_get_apibase(ctx),
		                   e_owner, e_repo, suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for Gitea repo");
	} break;
	}

	free(suffix);

	return rc;
}
