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

#include <gcli/forges.h>
#include <gcli/github/issues.h>
#include <gcli/issues.h>
#include <gcli/json_util.h>
#include <sn/sn.h>

void
gcli_issue_free(struct gcli_issue *const it)
{
	free(it->product);
	free(it->component);
	free(it->author);
	free(it->state);
	free(it->body);
	free(it->url);
	free(it->title);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i]);

	free(it->labels);
	it->labels = NULL;

	for (size_t i = 0; i < it->assignees_size; ++i)
		free(it->assignees[i]);

	free(it->assignees);
	it->assignees = NULL;

	free(it->milestone);
}

void
gcli_issues_free(struct gcli_issue_list *const list)
{
	for (size_t i = 0; i < list->issues_size; ++i)
		gcli_issue_free(&list->issues[i]);

	free(list->issues);

	list->issues = NULL;
	list->issues_size = 0;
}

int
gcli_issues_search(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   struct gcli_issue_fetch_details const *details, int const max,
                   struct gcli_issue_list *const out)
{
	gcli_null_check_call(search_issues, ctx, owner, repo, details, max, out);
}

int
gcli_get_issue(struct gcli_ctx *ctx, char const *owner, char const *repo,
               gcli_id const issue_number, struct gcli_issue *const out)
{
	gcli_null_check_call(get_issue_summary, ctx, owner, repo, issue_number,
	                     out);
}

int
gcli_issue_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const issue_number)
{
	gcli_null_check_call(issue_close, ctx, owner, repo, issue_number);
}

int
gcli_issue_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id const issue_number)
{
	gcli_null_check_call(issue_reopen, ctx, owner, repo, issue_number);
}

int
gcli_issue_submit(struct gcli_ctx *ctx, struct gcli_submit_issue_options *opts)
{
	gcli_null_check_call(perform_submit_issue, ctx, opts, NULL);
}

int
gcli_issue_assign(struct gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id const issue_number, char const *assignee)
{
	gcli_null_check_call(issue_assign, ctx, owner, repo, issue_number,
	                     assignee);
}

int
gcli_issue_add_labels(struct gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id const issue, char const *const labels[],
                      size_t const labels_size)
{
	gcli_null_check_call(issue_add_labels, ctx, owner, repo, issue, labels,
	                     labels_size);
}

int
gcli_issue_remove_labels(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id const issue,
                         char const *const labels[], size_t const labels_size)
{
	gcli_null_check_call(issue_remove_labels, ctx, owner, repo, issue,
	                     labels, labels_size);
}

int
gcli_issue_set_milestone(struct gcli_ctx *ctx, char const *const owner,
                         char const *const repo, gcli_id const issue,
                         int const milestone)
{
	gcli_null_check_call(issue_set_milestone, ctx, owner, repo, issue,
	                     milestone);
}

int
gcli_issue_clear_milestone(struct gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const issue)
{
	gcli_null_check_call(issue_clear_milestone, ctx, owner, repo, issue);
}

int
gcli_issue_set_title(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id issue, char const *new_title)
{
	gcli_null_check_call(issue_set_title, ctx, owner, repo, issue,
	                     new_title);
}

int
gcli_issue_get_attachments(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id issue,
                           struct gcli_attachment_list *out)
{
	gcli_null_check_call(get_issue_attachments, ctx, owner, repo, issue,
	                     out);
}
