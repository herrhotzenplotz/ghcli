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

#ifndef RELEASES_H
#define RELEASES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>
#include <sn/sn.h>

#include <time.h>

struct gcli_release_asset {
	char *name;
	char *url;
};

struct gcli_release {
	char *id; /* Probably shouldn't be called id */
	struct gcli_release_asset *assets;
	size_t assets_size;
	char *name;
	char *body;
	char *author;
	time_t date;
	char *upload_url;
	bool draft;
	bool prerelease;
};

struct gcli_release_list {
	struct gcli_release *releases;
	size_t releases_size;
};

struct gcli_release_asset_upload {
	char *label;
	char *name;
	char *path;
};

#define GCLI_RELEASE_MAX_ASSETS 16
struct gcli_new_release {
	char const *owner;
	char const *repo;
	char const *tag;
	char const *name;
	char *body;
	char const *commitish;
	bool draft;
	bool prerelease;
	struct gcli_release_asset_upload assets[GCLI_RELEASE_MAX_ASSETS];
	size_t assets_size;
};

int gcli_get_releases(struct gcli_ctx *ctx, struct gcli_path const *repo_path,
                      int max, struct gcli_release_list *list);

void gcli_free_releases(struct gcli_release_list *);

int gcli_create_release(struct gcli_ctx *ctx, struct gcli_new_release const *);

int gcli_release_push_asset(struct gcli_ctx *, struct gcli_new_release *,
                            struct gcli_release_asset_upload);

int gcli_delete_release(struct gcli_ctx *ctx, struct gcli_path const *repo_path,
                        char const *id);

void gcli_release_free(struct gcli_release *release);

#endif /* RELEASES_H */
