/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/forges.h>
#include <gcli/github/releases.h>
#include <gcli/releases.h>

#include <assert.h>
#include <stdlib.h>

int
gcli_get_releases(struct gcli_ctx *ctx, char const *owner, char const *repo,
                  int const max, struct gcli_release_list *const list)
{
	gcli_null_check_call(get_releases, ctx, owner, repo, max, list);
}

void
gcli_release_free(struct gcli_release *release)
{
	free(release->id);
	free(release->name);
	free(release->body);
	free(release->author);
	free(release->upload_url);

	for (size_t i = 0; i < release->assets_size; ++i) {
		free(release->assets[i].name);
		free(release->assets[i].url);
	}

	free(release->assets);
}

void
gcli_free_releases(struct gcli_release_list *const list)
{
	for (size_t i = 0; i < list->releases_size; ++i) {
		gcli_release_free(&list->releases[i]);
	}

	free(list->releases);

	list->releases = NULL;
	list->releases_size = 0;
}

int
gcli_create_release(struct gcli_ctx *ctx, struct gcli_new_release const *release)
{
	gcli_null_check_call(create_release, ctx, release);
}

int
gcli_release_push_asset(struct gcli_ctx *ctx, struct gcli_new_release *const release,
                        struct gcli_release_asset_upload const asset)
{
	if (release->assets_size == GCLI_RELEASE_MAX_ASSETS)
		return gcli_error(ctx, "too many assets");

	release->assets[release->assets_size++] = asset;

	return 0;
}

int
gcli_delete_release(struct gcli_ctx *ctx, char const *const owner,
                    char const *const repo, char const *const id)
{
	gcli_null_check_call(delete_release, ctx, owner, repo, id);
}
