/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/github/checks.h>
#include <gcli/github/pulls.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <sn/sn.h>

#include <assert.h>

void
gcli_pulls_free(struct gcli_pull_list *const it)
{
	for (size_t i = 0; i < it->pulls_size; ++i)
		gcli_pull_free(&it->pulls[i]);

	free(it->pulls);

	it->pulls = NULL;
	it->pulls_size = 0;
}

int
gcli_search_pulls(struct gcli_ctx *ctx, struct gcli_path const *const path,
                  struct gcli_pull_fetch_details const *const details,
                  int const max, struct gcli_pull_list *const out)
{
	gcli_null_check_call(search_pulls, ctx, path, details, max, out);
}

int
gcli_pull_get_diff(struct gcli_ctx *ctx, FILE *stream,
                   struct gcli_path const *const path)
{
	gcli_null_check_call(pull_get_diff, ctx, stream, path);
}

int
gcli_pull_get_commits(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      struct gcli_commit_list *const out)
{
	gcli_null_check_call(get_pull_commits, ctx, path, out);
}

void
gcli_commits_free(struct gcli_commit_list *list)
{
	for (size_t i = 0; i < list->commits_size; ++i) {
		free(list->commits[i].sha);
		free(list->commits[i].long_sha);
		free(list->commits[i].message);
		free(list->commits[i].date);
		free(list->commits[i].author);
		free(list->commits[i].email);
	}

	free(list->commits);

	list->commits = NULL;
	list->commits_size = 0;
}

void
gcli_pull_free(struct gcli_pull *const it)
{
	free(it->author);
	free(it->state);
	free(it->title);
	free(it->body);
	free(it->commits_link);
	free(it->head_label);
	free(it->base_label);
	free(it->head_sha);
	free(it->base_sha);
	free(it->milestone);
	free(it->coverage);
	free(it->node_id);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i]);

	free(it->labels);
	free(it->web_url);
}

int
gcli_get_pull(struct gcli_ctx *ctx, struct gcli_path const *const path,
              struct gcli_pull *const out)
{
	gcli_null_check_call(get_pull, ctx, path, out);
}

int
gcli_pull_get_checks(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     struct gcli_pull_checks_list *out)
{
	gcli_null_check_call(get_pull_checks, ctx, path, out);
}

void
gcli_pull_checks_free(struct gcli_pull_checks_list *list)
{
	switch (list->forge_type) {
	case GCLI_FORGE_GITHUB:
		github_free_checks((struct github_check_list *)list);
		break;
	case GCLI_FORGE_GITLAB:
		gitlab_pipelines_free((struct gitlab_pipeline_list *)list);
		break;
	default:
		assert(0 && "unreachable");
	}
}

int
gcli_pull_submit(struct gcli_ctx *ctx, struct gcli_submit_pull_options *opts)
{
	if (opts->automerge) {
		int const q = gcli_forge(ctx)->pull_summary_quirks;
		if (q & GCLI_PRS_QUIRK_AUTOMERGE)
			return gcli_error(ctx, "forge does not support auto-merge");
	}

	gcli_null_check_call(perform_submit_pull, ctx, opts);
}

int
gcli_pull_merge(struct gcli_ctx *ctx, struct gcli_path const *const path,
                enum gcli_merge_flags flags)
{
	gcli_null_check_call(pull_merge, ctx, path, flags);
}

int
gcli_pull_close(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(pull_close, ctx, path);
}

int
gcli_pull_reopen(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(pull_reopen, ctx, path);
}

int
gcli_pull_add_labels(struct gcli_ctx *ctx,
                     struct gcli_path const *const pull_path,
                     char const *const labels[], size_t const labels_size)
{
	gcli_null_check_call(pull_add_labels, ctx, pull_path, labels,
	                     labels_size);
}

int
gcli_pull_remove_labels(struct gcli_ctx *ctx,
                        struct gcli_path const *const pull_path,
                        char const *const labels[], size_t const labels_size)
{
	gcli_null_check_call(pull_remove_labels, ctx, pull_path, labels,
	                     labels_size);
}

int
gcli_pull_set_milestone(struct gcli_ctx *ctx,
                        struct gcli_path const *const pull_path,
                        int milestone_id)
{
	gcli_null_check_call(pull_set_milestone, ctx, pull_path, milestone_id);
}

int
gcli_pull_clear_milestone(struct gcli_ctx *ctx,
                          struct gcli_path const *const pull_path)
{
	gcli_null_check_call(pull_clear_milestone, ctx, pull_path);
}

int
gcli_pull_add_reviewer(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       char const *username)
{
	gcli_null_check_call(pull_add_reviewer, ctx, path, username);
}

int
gcli_pull_get_patch(struct gcli_ctx *ctx, FILE *out,
                    struct gcli_path const *const path)
{
	gcli_null_check_call(pull_get_patch, ctx, out, path);
}

int
gcli_pull_set_title(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    char const *new_title)
{
	gcli_null_check_call(pull_set_title, ctx, path, new_title);
}

int
gcli_pull_create_review(struct gcli_ctx *ctx,
                        struct gcli_pull_create_review_details const *details)
{
	gcli_null_check_call(pull_create_review, ctx, details);
}

char const *
gcli_pull_get_meta_by_key(struct gcli_pull_create_review_details const *details,
                          char const *key)
{
	size_t const key_len = strlen(key);
	struct gcli_review_meta_line *l;

	TAILQ_FOREACH(l, &details->meta_lines, next) {
		if (strncmp(l->entry, key, key_len) == 0 &&
		    l->entry[key_len] == ' ')
			return l->entry + key_len + 1;
	}

	return NULL;
}

int
gcli_pull_checkout(struct gcli_ctx *ctx, char const *const remote,
                   struct gcli_path const *const pull_path)
{
	gcli_null_check_call(pull_checkout, ctx, remote, pull_path);
}
