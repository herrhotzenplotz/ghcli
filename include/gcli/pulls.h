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

#ifndef PULLS_H
#define PULLS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>

#include <gcli/diffutil.h>
#include <gcli/gcli.h>
#include <sn/sn.h>

struct gcli_pull_list {
	struct gcli_pull *pulls;
	size_t pulls_size;
};

struct gcli_pull {
	char *author;
	char *state;
	char *title;
	char *body;
	char *created_at;
	char *commits_link;
	char *head_label;
	char *base_label;
	char *head_sha;
	char *base_sha;
	char *milestone;
	gcli_id id;
	gcli_id number;
	char *node_id;              /* Github: GraphQL compat */
	int comments;
	int additions;
	int deletions;
	int commits;
	int changed_files;
	int head_pipeline_id;       /* GitLab specific */
	char *coverage;             /* Gitlab Specific */
	char *web_url;

	char **labels;
	size_t labels_size;

	char **reviewers;      /**< User names */
	size_t reviewers_size; /**< Number of elements in the reviewers list */

	bool merged;
	bool mergeable;
	bool draft;
	bool automerge;
};

struct gcli_commit {
	char *sha, *long_sha, *message, *date, *author, *email;
};

struct gcli_commit_list {
	struct gcli_commit *commits;
	size_t commits_size;
};

/* Options to submit to the gh api for creating a PR */
struct gcli_submit_pull_options {
	char const *owner;
	char const *repo;
	char const *from;
	char const *to;
	char const *title;
	char *body;
	char **labels;
	size_t labels_size;
	int draft;
	bool automerge;           /** Automatically merge the PR when a pipeline passes */
};

struct gcli_pull_fetch_details {
	bool all;               /** Ignore status of the pull requests */
	char const *author;     /** Author of the pull request or NULL */
	char const *label;      /** a label attached to the pull request or NULL */
	char const *milestone;  /** a milestone this pull request is a part of or NULL */
	char const *search_term; /** some text to match in the pull request or NULL */
};

enum {
	GCLI_REVIEW_ACCEPT_CHANGES = 1,
	GCLI_REVIEW_REQUEST_CHANGES = 2,
	GCLI_REVIEW_COMMENT = 3,
};

struct gcli_pull_create_review_details {
	char const *owner, *repo;
	gcli_id pull_id;
	struct gcli_diff_comments comments;
	char *body;       /* string containing the prelude message by the user */
	char *gcli_meta;  /* string containing gcli metadata and commands */
	int review_state;
};

/** Generic list of checks ran on a pull request
 *
 * NOTE: KEEP THIS ORDER! WE DEPEND ON THE ABI HERE.
 *
 * For github the type of checks is gitlab_check*
 * For gitlab the type of checks is struct gitlab_pipeline*
 *
 * You can cast this type to the list type of either one of them. */
struct gcli_pull_checks_list {
	void *checks;
	size_t checks_size;
	int forge_type;
};

int gcli_search_pulls(struct gcli_ctx *ctx, char const *owner, char const *repo,
                      struct gcli_pull_fetch_details const *details, int max,
                      struct gcli_pull_list *out);

void gcli_pull_free(struct gcli_pull *it);

void gcli_pulls_free(struct gcli_pull_list *list);

int gcli_pull_get_diff(struct gcli_ctx *ctx, FILE *fout, char const *owner,
                       char const *repo, gcli_id pr_number);

int gcli_pull_get_checks(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id pr_number,
                         struct gcli_pull_checks_list *out);

void gcli_pull_checks_free(struct gcli_pull_checks_list *list);

int gcli_pull_get_commits(struct gcli_ctx *ctx, char const *owner,
                          char const *repo, gcli_id pr_number,
                          struct gcli_commit_list *out);

void gcli_commits_free(struct gcli_commit_list *list);

int gcli_get_pull(struct gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id pr_number, struct gcli_pull *out);

int gcli_pull_submit(struct gcli_ctx *ctx, struct gcli_submit_pull_options *);

enum gcli_merge_flags {
	GCLI_PULL_MERGE_SQUASH = 0x1, /* squash commits when merging */
	GCLI_PULL_MERGE_DELETEHEAD = 0x2, /* delete the source branch after merging */
};

int gcli_pull_merge(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id pr_number, enum gcli_merge_flags flags);

int gcli_pull_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id pr_number);

int gcli_pull_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id pr_number);

int gcli_pull_add_labels(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id pr_number,
                         char const *const labels[], size_t labels_size);

int gcli_pull_remove_labels(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id pr_number,
                            char const *const labels[], size_t labels_size);

int gcli_pull_set_milestone(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id pr_number,
                            int milestone_id);

int gcli_pull_clear_milestone(struct gcli_ctx *ctx, char const *owner,
                              char const *repo, gcli_id pr_number);

int gcli_pull_add_reviewer(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id pr_number,
                           char const *username);

int gcli_pull_get_patch(struct gcli_ctx *ctx, FILE *out, char const *owner,
                        char const *repo, gcli_id pr_number);

int gcli_pull_set_title(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id pull, char const *new_title);

int gcli_pull_create_review(struct gcli_ctx *ctx,
                            struct gcli_pull_create_review_details const *details);

int gcli_pull_get_patch(struct gcli_ctx *ctx, FILE *out, char const *owner,
                        char const *repo, gcli_id pr_number);

#endif /* PULLS_H */
