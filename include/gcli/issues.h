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

#ifndef ISSUES_H
#define ISSUES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>
#include <gcli/nvlist.h>
#include <gcli/path.h>

#include <sn/sn.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <gcli/attachments.h>

struct gcli_issue {
	gcli_id number;
	char *title;
	char *product;     /* only on Bugzilla */
	char *component;   /* only on Bugzilla */
	char *url;         /* only on Bugzilla */
	time_t created_at;
	char *author;
	char *state;
	int comments;
	bool locked;
	char *body;
	char **labels;
	size_t labels_size;
	char **assignees;
	size_t assignees_size;
	/* workaround for GitHub where PRs are also issues */
	int is_pr;
	char *milestone;
};

struct gcli_submit_issue_options {
	char const *owner;
	char const *repo;
	char *title;
	char *body;

	struct gcli_nvlist extra;
};

struct gcli_issue_list {
	struct gcli_issue *issues;
	size_t issues_size;
};

struct gcli_issue_fetch_details {
	bool all;                   /* disregard the issue state */
	char const *author;         /* filter issues by this author*/
	char const *label;          /* filter by the given label */
	char const *milestone;      /* filter by the given milestone */
	char const *search_term;    /* a search term or NULL if unspecified */
};

int gcli_issues_search(struct gcli_ctx *ctx, struct gcli_path const *path,
                       struct gcli_issue_fetch_details const *details, int max,
                       struct gcli_issue_list *out);

void gcli_issues_free(struct gcli_issue_list *);

int gcli_get_issue(struct gcli_ctx *ctx, struct gcli_path const *path,
                   struct gcli_issue *out);

void gcli_issue_free(struct gcli_issue *it);

int gcli_issue_close(struct gcli_ctx *ctx, struct gcli_path const *path);

int gcli_issue_reopen(struct gcli_ctx *ctx, struct gcli_path const *path);

int gcli_issue_submit(struct gcli_ctx *ctx, struct gcli_submit_issue_options *);

int gcli_issue_assign(struct gcli_ctx *ctx, struct gcli_path const *issue_path,
                      char const *assignee);

int gcli_issue_add_labels(struct gcli_ctx *ctx,
                          struct gcli_path const *issue_path,
                          char const *const labels[], size_t labels_size);

int gcli_issue_remove_labels(struct gcli_ctx *ctx,
                             struct gcli_path const *issue_path,
                             char const *const labels[], size_t labels_size);

int gcli_issue_set_milestone(struct gcli_ctx *ctx,
                             struct gcli_path const *const path,
                             int milestone);

int gcli_issue_clear_milestone(struct gcli_ctx *cxt,
                               struct gcli_path const *const issue_path);

int gcli_issue_set_title(struct gcli_ctx *ctx,
                         struct gcli_path const *issue_path,
                         char const *new_title);

int gcli_issue_get_attachments(struct gcli_ctx *ctx,
                               struct gcli_path const *const path,
                               struct gcli_attachment_list *attachments);

#endif /* ISSUES_H */
