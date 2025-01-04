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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/releases.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/gitlab/releases.h>

#include <pdjson/pdjson.h>

#include <string.h>

static void
fixup_asset_name(struct gcli_ctx *ctx, struct gcli_release_asset *const asset)
{
	if (!asset->name)
		asset->name = gcli_urldecode(ctx, strrchr(asset->url, '/') + 1);
}

void
gitlab_fixup_release_assets(struct gcli_ctx *ctx, struct gcli_release *const release)
{
	for (size_t i = 0; i < release->assets_size; ++i)
		fixup_asset_name(ctx, &release->assets[i]);
}

static void
fixup_release_asset_names(struct gcli_ctx *ctx, struct gcli_release_list *list)
{
	/* Iterate over releases */
	for (size_t i = 0; i < list->releases_size; ++i)
		gitlab_fixup_release_assets(ctx, &list->releases[i]);
}

int
gitlab_get_releases(struct gcli_ctx *ctx,
                    struct gcli_path const *const repo_path, int const max,
                    struct gcli_release_list *const list)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &list->releases,
		.sizep = &list->releases_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_releases),
	};

	*list = (struct gcli_release_list) {0};

	rc = gitlab_repo_make_url(ctx, repo_path, &url, "/releases");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_list(ctx, url, &fl);

	if (rc == 0)
		fixup_release_asset_names(ctx, list);

	return rc;
}

int
gitlab_create_release(struct gcli_ctx *ctx, struct gcli_new_release const *release)
{
	char *e_owner = NULL, *e_repo = NULL, *url = NULL, *payload = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Warnings because unsupported on gitlab */
	if (release->prerelease)
		warnx("prereleases are not supported on GitLab, option ignored");

	if (release->draft)
		warnx("draft releases are not supported on GitLab, option ignored");

	if (release->assets_size)
		warnx("GitLab release asset uploads are not yet supported");

	/* Payload generation */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "tag_name");
		gcli_jsongen_string(&gen, release->tag);

		if (release->body) {
			gcli_jsongen_objmember(&gen, "description");
			gcli_jsongen_string(&gen, release->body);
		}

		if (release->commitish) {
			gcli_jsongen_objmember(&gen, "ref");
			gcli_jsongen_string(&gen, release->commitish);
		}

		if (release->name) {
			gcli_jsongen_objmember(&gen, "name");
			gcli_jsongen_string(&gen, release->name);
		}
	}
	gcli_jsongen_end_object(&gen);
	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(release->owner);
	e_repo = gcli_urlencode(release->repo);

	/* https://docs.github.com/en/rest/reference/repos#create-a-release */
	url = sn_asprintf("%s/projects/%s%%2F%s/releases", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
gitlab_delete_release(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char const *const id)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_repo_make_url(ctx, path, &url, "/releases/%s", id);
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

	return rc;
}
