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

#include <gcli/gitea/releases.h>
#include <gcli/github/releases.h>

#include <gcli/curl.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/github/releases.h>

int
gitea_get_releases(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   int const max, struct gcli_release_list *const list)
{
	return github_get_releases(ctx, owner, repo, max, list);
}

static void
gitea_parse_release(struct gcli_ctx *ctx, struct gcli_fetch_buffer const *const buffer,
                    struct gcli_release *const out)
{
	struct json_stream stream = {0};
	json_open_buffer(&stream, buffer->data, buffer->length);
	parse_github_release(ctx, &stream, out);
	json_close(&stream);
}

static int
gitea_upload_release_asset(struct gcli_ctx *ctx, char *const url,
                           struct gcli_release_asset_upload const asset)
{
	char *e_assetname = NULL;
	char *request = NULL;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	e_assetname = gcli_urlencode(asset.name);
	request = sn_asprintf("%s?name=%s", url, e_assetname);

	rc = gcli_curl_gitea_upload_attachment(ctx, request, asset.path, &buffer);

	free(request);
	free(e_assetname);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
gitea_create_release(struct gcli_ctx *ctx, struct gcli_new_release const *release)
{
	char *e_owner = NULL, *e_repo = NULL, *payload = NULL, *upload_url = NULL, *url = NULL;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	struct gcli_release response = {0};
	int rc = 0;

	/* Payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "tag_name");
		gcli_jsongen_string(&gen, release->tag);

		gcli_jsongen_objmember(&gen, "draft");
		gcli_jsongen_bool(&gen, release->draft);

		gcli_jsongen_objmember(&gen, "prerelease");
		gcli_jsongen_bool(&gen, release->prerelease);

		if (release->body) {
			gcli_jsongen_objmember(&gen, "body");
			gcli_jsongen_string(&gen, release->body);
		}

		if (release->commitish) {
			gcli_jsongen_objmember(&gen, "target_commitish");
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
	url = sn_asprintf("%s/repos/%s/%s/releases", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);
	if (rc < 0)
		goto out;

	gitea_parse_release(ctx, &buffer, &response);

	upload_url = sn_asprintf("%s/repos/%s/%s/releases/%s/assets",
	                         gcli_get_apibase(ctx), e_owner, e_repo,
	                         response.id);

	for (size_t i = 0; i < release->assets_size; ++i) {
		printf("INFO : Uploading asset %s...\n", release->assets[i].path);
		rc = gitea_upload_release_asset(ctx, upload_url, release->assets[i]);

		if (rc < 0)
			break;
	}

	gcli_release_free(&response);
out:
	gcli_fetch_buffer_free(&buffer);
	free(e_owner);
	free(e_repo);
	free(upload_url);
	free(url);
	free(payload);

	return rc;
}

int
gitea_delete_release(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     char const *id)
{
	return github_delete_release(ctx, owner, repo, id);
}
