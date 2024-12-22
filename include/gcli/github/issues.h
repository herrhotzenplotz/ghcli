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

#ifndef GCLI_ISSUES_H
#define GCLI_ISSUES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/curl.h>
#include <gcli/issues.h>

int github_issue_make_url(struct gcli_ctx *ctx, struct gcli_path const *,
                          char **url, char const *suffix_fmt, ...);

int github_fetch_issues(struct gcli_ctx *ctx, char *url, int max,
                        struct gcli_issue_list *out);

int github_issues_search(struct gcli_ctx *ctx, struct gcli_path const *const path,
                         struct gcli_issue_fetch_details const *details, int max,
                         struct gcli_issue_list *out);

int github_fetch_issue(struct gcli_ctx *ctx, char *url, struct gcli_issue *out);

int github_get_issue_summary(struct gcli_ctx *ctx, struct gcli_path const *path,
                             struct gcli_issue *out);

int github_issue_close(struct gcli_ctx *ctx, struct gcli_path const *path);

int github_issue_reopen(struct gcli_ctx *ctx, struct gcli_path const *path);

int github_perform_submit_issue(struct gcli_ctx *ctx,
                                struct gcli_submit_issue_options *opts,
                                struct gcli_issue *out);

int github_issue_assign(struct gcli_ctx *ctx, struct gcli_path const *issue_path,
                        char const *assignee);

int github_issue_add_labels(struct gcli_ctx *ctx,
                            struct gcli_path const *issue_path,
                            char const *const labels[], size_t labels_size);

int github_issue_remove_labels(struct gcli_ctx *ctx,
                               struct gcli_path const *path,
                               char const *const labels[], size_t labels_size);

int github_issue_set_milestone(struct gcli_ctx *ctx,
                               struct gcli_path const *issue_path,
                               gcli_id milestone);

int github_issue_clear_milestone(struct gcli_ctx *ctx,
                                 struct gcli_path const *issue_path);

int github_issue_set_title(struct gcli_ctx *ctx,
                           struct gcli_path const *const issue_path,
                           char const *const new_title);

#endif /* GCLI_ISSUES_H */
