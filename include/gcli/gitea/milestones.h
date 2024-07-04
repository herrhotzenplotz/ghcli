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

#ifndef GCLI_GITEA_MILESTONES_H
#define GCLI_GITEA_MILESTONES_H

#include <gcli/milestones.h>

int gitea_get_milestones(struct gcli_ctx *ctx,
                         struct gcli_path const *path, int max,
                         struct gcli_milestone_list *out);

int gitea_get_milestone(struct gcli_ctx *ctx, struct gcli_path const *path,
                        struct gcli_milestone *out);

int gitea_create_milestone(struct gcli_ctx *ctx,
                           struct gcli_milestone_create_args const *args);

int gitea_delete_milestone(struct gcli_ctx *ctx,
                           struct gcli_path const *const path);

int gitea_milestone_set_duedate(struct gcli_ctx *ctx,
                                struct gcli_path const *path,
                                char const *date);

int gitea_milestone_get_issues(struct gcli_ctx *ctx,
                               struct gcli_path const *path,
                               struct gcli_issue_list *out);

#endif /* GCLI_GITEA_MILESTONES_H */
