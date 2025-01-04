/*
 * Copyright 2021-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/curl.h>
#include <gcli/diffutil.h>
#include <gcli/github/checks.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/pulls.h>
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/pulls.h>

#include <stdarg.h>

int
github_pull_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     char **url, char const *const fmt, ...)
{
	int rc = 0;
	char *suffix = NULL;
	va_list vp;

	va_start(vp, fmt);
	suffix = sn_vasprintf(fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s/pulls/%"PRIid"%s",
		                   gcli_get_apibase(ctx),
		                   e_owner, e_repo, path->data.as_default.id,
		                   suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab merge request");
	} break;
	}

	free(suffix);

	return rc;
}

/* The following function is a hack around the stupidity of the Github
 * REST API. With Github's REST API it is impossible to explicitly
 * request a list of pull requests filtered by a given author. I guess
 * what you're supposed to be doing is to do the filtering
 * yourself.
 *
 * What this function does is to go through the list of pull requests
 * and removes the ones that are not authored by the given
 * username. It then shrinks the allocation size of the list such that
 * we don't confuse the malloc allocator. Really, it shouldn't change
 * the actual storage and instead just record the new size of the
 * allocation. */

static bool
pull_has_label(struct gcli_pull const *p, char const *const label)
{
	for (size_t i = 0; i < p->labels_size; ++i) {
		if (strcmp(p->labels[i], label) == 0)
			return true;
	}
	return false;
}

static void
github_pulls_filter(struct gcli_pull **listp, size_t *sizep,
                    struct gcli_pull_fetch_details const *details)
{
	for (size_t i = *sizep; i > 0; --i) {
		struct gcli_pull *pulls = *listp;
		struct gcli_pull *pull = &pulls[i-1];
		bool should_remove = false;

		if (details->author && strcmp(details->author, pull->author))
			should_remove = true;

		if (details->label && !pull_has_label(pull, details->label))
			should_remove = true;

		if (details->milestone && pull->milestone &&
		    strcmp(pull->milestone, details->milestone))
			should_remove = true;

		if (should_remove) {
			gcli_pull_free(pull);

			memmove(pull, &pulls[i], sizeof(*pulls) * (*sizep - i));
			*listp = realloc(pulls, sizeof(*pulls) * (--(*sizep)));
		}
	}
}

static int
github_fetch_pulls(struct gcli_ctx *ctx, char *url,
                   struct gcli_pull_fetch_details const *details, int max,
                   struct gcli_pull_list *const list)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &list->pulls,
		.sizep = &list->pulls_size,
		.parse = (parsefn)(parse_github_pulls),
		.filter = (filterfn)(github_pulls_filter),
		.userdata = details,
		.max = max,
	};

	return gcli_fetch_list(ctx, url, &fl);
}

static int
search_pulls(struct gcli_ctx *ctx, struct gcli_path const *const path,
             struct gcli_pull_fetch_details const *const details,
             int const max, struct gcli_pull_list *const out)
{
	char *url = NULL, *query_string = NULL, *e_query_string = NULL,
	     *milestone = NULL, *author = NULL, *label = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	(void) max;

	if (path->kind != GCLI_PATH_DEFAULT)
		return gcli_error(ctx, "unsupported path kind for searching "
		                  "with search term on GitHub");

	if (details->milestone)
		milestone = sn_asprintf("milestone:%s", details->milestone);

	if (details->author)
		author = sn_asprintf("author:%s", details->author);

	if (details->label)
		label = sn_asprintf("label:%s", details->label);

	query_string = sn_asprintf("repo:%s/%s is:pull-request%s %s %s %s %s",
	                           path->data.as_default.owner,
	                           path->data.as_default.repo,
	                           details->all ? "" : " is:open",
	                           milestone ? milestone : "", author ? author : "",
	                           label ? label : "", details->search_term);
	e_query_string = gcli_urlencode(query_string);

	url = sn_asprintf("%s/search/issues?q=%s", gcli_get_apibase(ctx),
	                  e_query_string);

	free(milestone);
	free(author);
	free(label);
	free(query_string);
	free(e_query_string);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_github_pull_search_result(ctx, &stream, out);

	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

static int
list_pulls(struct gcli_ctx *ctx, struct gcli_path const *const path,
           struct gcli_pull_fetch_details const *const details,
           int const max, struct gcli_pull_list *const list)
{
	char *url = NULL;
	int rc = 0;

	rc = github_repo_make_url(ctx, path, &url, "/pulls?state=%s",
	                          details->all ? "all" : "open");
	if (rc < 0)
		return rc;


	return github_fetch_pulls(ctx, url, details, max, list);
}

int
github_search_pulls(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_pull_fetch_details const *const details,
                    int const max, struct gcli_pull_list *const list)
{
	if (details->search_term)
		return search_pulls(ctx, path, details, max, list);
	else
		return list_pulls(ctx, path, details, max, list);
}

int
github_pull_get_patch(struct gcli_ctx *ctx, FILE *stream,
                      struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = github_pull_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_curl(ctx, stream, url, "Accept: application/vnd.github.v3.patch");

	free(url);

	return rc;
}

int
github_print_get_patch(struct gcli_ctx *ctx, FILE *stream, char const *owner,
                       char const *repo, gcli_id pr_number)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%"PRIid,
		gcli_get_apibase(ctx),
		e_owner, e_repo, pr_number);
	rc = gcli_curl(ctx, stream, url, "Accept: application/vnd.github.v3.patch");

	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
github_pull_get_diff(struct gcli_ctx *ctx, FILE *stream,
                     struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = github_pull_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_curl(ctx, stream, url, "Accept: application/vnd.github.v3.diff");

	free(url);

	return rc;
}

static int
github_pull_delete_head_branch(struct gcli_ctx *ctx,
                               struct gcli_path const *const path)
{
	struct gcli_pull pull = {0};
	char *url = NULL;
	char const *head_branch;
	int rc = 0;

	rc = github_get_pull(ctx, path, &pull);
	if (rc < 0)
		return rc;

	head_branch = strchr(pull.head_label, ':');
	head_branch++;

	rc = github_repo_make_url(ctx, path, &url, "/git/refs/heads/%s",
	                          head_branch);
	if (rc < 0)
		goto err_make_url;

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

err_make_url:
	gcli_pull_free(&pull);

	return rc;
}

int
github_pull_merge(struct gcli_ctx *ctx, struct gcli_path const *const path,
                  enum gcli_merge_flags const flags)
{
	bool const delete_source = flags & GCLI_PULL_MERGE_DELETEHEAD;
	bool const squash = flags & GCLI_PULL_MERGE_SQUASH;
	char *url = NULL;
	int rc = 0;

	rc = github_pull_make_url(ctx, path, &url, "/merge?merge_method=%s",
	                          squash ? "squash" : "merge");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "PUT", url, "{}", NULL, NULL);

	if (rc == 0 && delete_source)
		rc = github_pull_delete_head_branch(ctx, path);

	free(url);

	return rc;
}

static int
github_pull_patch_state(struct gcli_ctx *const ctx,
                        struct gcli_path const *const path,
                        char const *const new_state)
{
	char *url = NULL, *payload = NULL;
	int rc = 0;
	struct gcli_jsongen gen = {0};

	/* Generate URL */
	rc = github_pull_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "state");
		gcli_jsongen_string(&gen, new_state);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
github_pull_close(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return github_pull_patch_state(ctx, path, "closed");
}

int
github_pull_reopen(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return github_pull_patch_state(ctx, path, "open");
}

static int
github_pull_set_automerge(struct gcli_ctx *const ctx, char const *const node_id)
{
	char *url, *query, *payload;
	int rc;
	char const *const fmt =
		"mutation updateAutomergeState {\n"
		"   enablePullRequestAutoMerge(input: {\n"
		"       pullRequestId: \"%s\",\n"
		"       mergeMethod: MERGE\n"
		"   }) {\n"
		"      clientMutationId\n"
		"   }\n"
		"}\n";

	struct gcli_jsongen gen = {0};

	query = sn_asprintf(fmt, node_id);

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "query");
		gcli_jsongen_string(&gen, query);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);
	free(query);

	url = sn_asprintf("%s/graphql", gcli_get_apibase(ctx));

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}

static int
github_pull_add_reviewers(struct gcli_ctx *ctx,
                          struct gcli_path const *const path,
                          char const *const *users, size_t users_size)
{
	int rc = 0;
	char *url, *payload;
	struct gcli_jsongen gen = {0};

	/* /repos/{owner}/{repo}/pulls/{pull_number}/requested_reviewers */
	rc = github_pull_make_url(ctx, path, &url, "/requested_reviewers");
	if (rc < 0)
		return rc;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "reviewers");

		gcli_jsongen_begin_array(&gen);
		for (size_t i = 0; i < users_size; ++i)
			gcli_jsongen_string(&gen, users[i]);

		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Perform request */
	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	/* Cleanup */
	free(payload);
	free(url);

	return rc;
}

int
github_perform_submit_pull(struct gcli_ctx *ctx,
                           struct gcli_submit_pull_options *opts)
{
	char *url = NULL, *payload = NULL;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	int rc = 0;

	rc = github_repo_make_url(ctx, &opts->target_repo, &url, "/pulls");
	if (rc < 0)
		return rc;

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "head");
		gcli_jsongen_string(&gen, opts->from);

		gcli_jsongen_objmember(&gen, "base");
		gcli_jsongen_string(&gen, opts->target_branch);

		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, opts->title);

		/* Body is optional and will be NULL if unset */
		if (opts->body) {
			gcli_jsongen_objmember(&gen, "body");
			gcli_jsongen_string(&gen, opts->body);
		}
	}
	gcli_jsongen_end_object(&gen);
	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);


	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);

	/* Add labels or reviewers if requested or set automerge.
	 * GitHub doesn't allow us to do this all with one request. */
	if (rc == 0 &&
	    (opts->labels_size || opts->automerge || opts->reviewers_size) &&
	    (opts->target_repo.kind == GCLI_PATH_DEFAULT))
	{
		struct json_stream json = {0};
		struct gcli_pull pull = {0};
		struct gcli_path target_pull_path = {0};

		json_open_buffer(&json, buffer.data, buffer.length);
		parse_github_pull(ctx, &json, &pull);

		target_pull_path = opts->target_repo;
		target_pull_path.data.as_default.id = pull.id;

		if (opts->labels_size) {
			rc = github_issue_add_labels(
				ctx, &target_pull_path,
				(char const *const *)opts->labels,
				opts->labels_size);
		}

		if (rc == 0 && opts->reviewers_size) {
			rc = github_pull_add_reviewers(
				ctx, &target_pull_path,
				(char const *const *)opts->reviewers,
				opts->reviewers_size);
		}

		if (rc == 0 && opts->automerge) {
			/* pull.id is the global pull request ID */
			rc = github_pull_set_automerge(ctx, pull.node_id);
		}

		gcli_pull_free(&pull);
		json_close(&json);
	}

	gcli_fetch_buffer_free(&buffer);
	free(payload);
	free(url);

	return rc;
}

static void
filter_commit_short_sha(struct gcli_commit **listp, size_t *sizep, void *_data)
{
	(void) _data;

	for (size_t i = 0; i < *sizep; ++i)
		(*listp)[i].sha = sn_strndup((*listp)[i].long_sha, 8);
}

int
github_get_pull_commits(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        struct gcli_commit_list *const out)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->commits,
		.sizep = &out->commits_size,
		.max = -1,
		.parse = (parsefn)(parse_github_commits),
		.filter = (filterfn)(filter_commit_short_sha),
	};

	rc = github_pull_make_url(ctx, path, &url, "/commits");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_get_pull(struct gcli_ctx *ctx, struct gcli_path const *const path,
                struct gcli_pull *const out)
{
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	char *url = NULL;

	rc = github_pull_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_pull(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_pull_get_checks(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       struct gcli_pull_checks_list *out)
{
	char refname[64] = {0};

	if (path->kind != GCLI_PATH_DEFAULT)
		return gcli_error(ctx, "unsupported path kind for GitHub Checks");

	/* This is kind of a hack, but it works!
	 * Yes, even a few months later I agree that this is a hack. */
	snprintf(refname, sizeof refname, "refs%%2Fpull%%2F%"PRIid"%%2Fhead",
	         path->data.as_default.id);

	return github_get_checks(ctx, path, refname, -1, (struct github_check_list *)out);
}

int
github_pull_add_reviewer(struct gcli_ctx *ctx,
                         struct gcli_path const *const path,
                         char const *username)
{
	return github_pull_add_reviewers(ctx, path, &username, 1);
}

int
github_pull_set_title(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char const *new_title)
{
	char *url, *payload;
	int rc;
	struct gcli_jsongen gen = {0};

	/* Generate the url */
	rc = github_pull_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	/* Generate the payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, new_title);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	/* Cleanup */
	free(payload);
	free(url);

	return rc;
}

int
github_pull_create_review(struct gcli_ctx *ctx,
                          struct gcli_pull_create_review_details const *details)
{
	int rc = 0;
	char *url = NULL, *payload = NULL;
	struct gcli_jsongen gen = {0};

	rc = github_pull_make_url(ctx, &details->path, &url, "/reviews");
	if (rc < 0)
		return rc;

	if (gcli_jsongen_init(&gen) < 0) {
		rc = gcli_error(ctx, "failed to init JSON generator");
		goto bail;
	}

	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "body");
		gcli_jsongen_string(&gen, details->body);

		gcli_jsongen_objmember(&gen, "event");
		switch (details->review_state) {
		case GCLI_REVIEW_ACCEPT_CHANGES:
			gcli_jsongen_string(&gen, "APPROVE");
			break;
		case GCLI_REVIEW_REQUEST_CHANGES:
			gcli_jsongen_string(&gen, "REQUEST_CHANGES");
			break;
		case GCLI_REVIEW_COMMENT:
			gcli_jsongen_string(&gen, "COMMENT");
			break;
		default:
			rc = gcli_error(ctx, "bad review state: %d",
			                details->review_state);
			goto bail;
		}

		gcli_jsongen_objmember(&gen, "comments");
		gcli_jsongen_begin_array(&gen);
		{
			struct gcli_diff_comment *comment;

			TAILQ_FOREACH(comment, &details->comments, next) {
				gcli_jsongen_begin_object(&gen);
				{
					gcli_jsongen_objmember(&gen, "path");
					gcli_jsongen_string(&gen, comment->after.filename);

					gcli_jsongen_objmember(&gen, "body");
					gcli_jsongen_string(&gen, comment->comment);

					gcli_jsongen_objmember(&gen, "line");
					gcli_jsongen_number(&gen, comment->after.end_row);

					if (comment->after.start_row != comment->after.end_row) {
						gcli_jsongen_objmember(&gen, "start_line");
						gcli_jsongen_number(&gen, comment->after.start_row);
					}
				}
				gcli_jsongen_end_object(&gen);
			}
		}
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	gcli_jsongen_free(&gen);

bail:
	free(payload);
	free(url);

	return rc;
}
