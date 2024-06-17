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

#ifndef GITLAB_MERGE_REQUESTS_H
#define GITLAB_MERGE_REQUESTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/pulls.h>

struct gitlab_reviewer_id_list {
	gcli_id *reviewers;
	size_t reviewers_size;
};

/* Structs used for internal patch generator. Gitlab does not provide
 * an endpoint for doing this properly. */
struct gitlab_diff {
	char *diff;
	char *old_path;
	char *new_path;
	char *a_mode;
	char *b_mode;
	bool new_file;
	bool renamed_file;
	bool deleted_file;
};

struct gitlab_diff_list {
	struct gitlab_diff *diffs;
	size_t diffs_size;
};

struct gitlab_mr_version {
	gcli_id id;
	char *head_commit, *base_commit, *start_commit;
};

struct gitlab_mr_version_list {
	struct gitlab_mr_version *versions;
	size_t versions_size;
};

int gitlab_fetch_mrs(struct gcli_ctx *ctx, char *url, int max,
                     struct gcli_pull_list *list);

int gitlab_get_mrs(struct gcli_ctx *ctx, char const *owner,
                   char const *reponame,
                   struct gcli_pull_fetch_details const *details,
                   int max,
                   struct gcli_pull_list *out);

int gitlab_mr_get_diff(struct gcli_ctx *ctx, FILE *stream, char const *owner,
                       char const *reponame, gcli_id mr_number);

int gitlab_mr_get_patch(struct gcli_ctx *ctx, FILE *stream, char const *owner,
                        char const *reponame, gcli_id mr_number);

int gitlab_mr_merge(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id mr_number, enum gcli_merge_flags flags);

int gitlab_mr_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id mr_number);

int gitlab_mr_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id mr_number);

int gitlab_get_pull(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id mr_number, struct gcli_pull *out);

int gitlab_get_pull_commits(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id mr_number,
                            struct gcli_commit_list *out);

int gitlab_perform_submit_mr(struct gcli_ctx *ctx, struct gcli_submit_pull_options *opts);

int gitlab_mr_add_labels(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id mr_number,
                         char const *const labels[], size_t labels_size);

int gitlab_mr_remove_labels(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id mr_number,
                            char const *const labels[], size_t labels_size);

int gitlab_mr_set_milestone(struct  gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id mr_number,
                            gcli_id milestone_id);

int gitlab_mr_clear_milestone(struct gcli_ctx *ctx, char const *owner,
                              char const *repo, gcli_id mr_number);

int gitlab_mr_add_reviewer(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id mr_number,
                           char const *username);

int gitlab_mr_set_title(struct gcli_ctx *ctx, char const *const owner,
                        char const *const repo, gcli_id const id,
                        char const *const new_title);

int gitlab_mr_create_review(struct gcli_ctx *ctx,
                            struct gcli_pull_create_review_details const *details);

void gitlab_mr_version_free(struct gitlab_mr_version *);

void gitlab_mr_version_list_free(struct gitlab_mr_version_list *);

#endif /* GITLAB_MERGE_REQUESTS_H */
