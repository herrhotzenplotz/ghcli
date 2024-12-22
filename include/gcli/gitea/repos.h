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

#ifndef GITEA_REPOS_H
#define GITEA_REPOS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/repos.h>

int gitea_get_repos(struct gcli_ctx *ctx, char const *owner, int max,
                    struct gcli_repo_list *out);

int gitea_get_own_repos(struct gcli_ctx *ctx, int max,
                        struct gcli_repo_list *out);

int gitea_repo_create(struct gcli_ctx *ctx,
                      struct gcli_repo_create_options const *options,
                      struct gcli_repo *out);

int gitea_repo_delete(struct gcli_ctx *ctx, struct gcli_path const *path);

int gitea_repo_set_visibility(struct gcli_ctx *ctx,
                              struct gcli_path const *path,
                              gcli_repo_visibility vis);

int gitea_repo_make_url(struct gcli_ctx *ctx, struct gcli_path const *path,
                        char **url, char const *suffix_fmt, ...);

#endif /* GITEA_REPOS_H */
