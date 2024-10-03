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
#include <gcli/gitlab/api.h>
#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/gitlab/merge_requests.h>

#include <pdjson/pdjson.h>

#include <time.h> /* for nanosleep */

#include <openssl/evp.h>

/* Workaround because gitlab doesn't give us an explicit field for
 * this. */
static void
gitlab_mrs_fixup(struct gcli_pull_list *const list)
{
	for (size_t i = 0; i < list->pulls_size; ++i) {
		list->pulls[i].merged = !strcmp(list->pulls[i].state, "merged");
	}
}

int
gitlab_fetch_mrs(struct gcli_ctx *ctx, char *url, int const max,
                 struct gcli_pull_list *const list)
{
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &list->pulls,
		.sizep = &list->pulls_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_mrs),
	};

	rc = gcli_fetch_list(ctx, url, &fl);

	/* TODO: don't leak the list on error */
	if (rc == 0)
		gitlab_mrs_fixup(list);

	return rc;
}

int
gitlab_get_mrs(struct gcli_ctx *ctx, char const *owner, char const *repo,
               struct gcli_pull_fetch_details const *const details, int const max,
               struct gcli_pull_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;
	char *e_label = NULL;
	char *e_milestone = NULL;
	char *e_search = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		bool const need_qmark = details->all;
		e_author = sn_asprintf("%cauthor_username=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->label) {
		char *tmp = gcli_urlencode(details->label);
		bool const need_qmark = details->all && !details->author;
		e_label = sn_asprintf("%clabels=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->milestone) {
		char *tmp = gcli_urlencode(details->milestone);
		bool const need_qmark = details->all && !details->author && !details->label;
		e_milestone = sn_asprintf("%cmilestone=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->search_term) {
		char *tmp = gcli_urlencode(details->search_term);
		bool const need_qmark = details->all && !details->author &&
			!details->label && !details->milestone;

		e_search = sn_asprintf("%csearch=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests%s%s%s%s%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo,
	                  details->all ? "" : "?state=opened",
	                  e_author ? e_author : "",
	                  e_label ? e_label : "",
	                  e_milestone ? e_milestone : "",
	                  e_search ? e_search : "");

	free(e_search);
	free(e_milestone);
	free(e_label);
	free(e_author);
	free(e_owner);
	free(e_repo);

	return gitlab_fetch_mrs(ctx, url, max, list);
}

static void
gitlab_free_diff(struct gitlab_diff *diff)
{
	free(diff->diff);
	free(diff->old_path);
	free(diff->new_path);
	free(diff->a_mode);
	free(diff->b_mode);

	memset(diff, 0, sizeof(*diff));
}

static void
gitlab_free_diffs(struct gitlab_diff_list *list)
{
	for (size_t i = 0; i < list->diffs_size; ++i) {
		gitlab_free_diff(&list->diffs[i]);
	}

	free(list->diffs);
	list->diffs = NULL;
	list->diffs_size = 0;
}

static void
gitlab_make_commit_diff(struct gcli_commit const *const commit,
                        struct gitlab_diff const *const diff,
                        char const *const prev_commit_sha, FILE *const out)
{
	fprintf(out, "diff --git a/%s b/%s\n", diff->old_path, diff->new_path);
	if (diff->new_file) {
		fprintf(out, "new file mode %s\n", diff->b_mode);
		fprintf(out, "index 0000000..%s\n", commit->sha);
	} else {
		fprintf(out, "index %s..%s %s\n", prev_commit_sha, commit->sha,
		        diff->b_mode);
	}

	fprintf(out, "--- %s%s\n",
	        diff->new_file ? "" : "a/",
	        diff->new_file ? "/dev/null" : diff->old_path);
	fprintf(out, "+++ %s%s\n",
	        diff->deleted_file ? "" : "b/",
	        diff->deleted_file ? "/dev/null" : diff->new_path);
	fputs(diff->diff, out);
}

static int
gitlab_make_commit_patch(struct gcli_ctx *ctx, FILE *stream,
                         char const *const e_owner, char const *const e_repo,
                         char const *const prev_commit_sha,
                         struct gcli_commit const *const commit)
{
	char *url;
	int rc;
	struct gitlab_diff_list list = {0};

	struct gcli_fetch_list_ctx fl = {
		.listp = &list.diffs,
		.sizep = &list.diffs_size,
		.max = -1,
		.parse = (parsefn)(parse_gitlab_diffs),
	};

	/* /projects/:id/repository/commits/:sha/diff */
	url = sn_asprintf("%s/projects/%s%%2F%s/repository/commits/%s/diff",
	                  gcli_get_apibase(ctx), e_owner, e_repo, commit->sha);

	rc = gcli_fetch_list(ctx, url, &fl);
	if (rc < 0)
		goto err_fetch_diffs;

	fprintf(stream, "From %s Mon Sep 17 00:00:00 2001\n", commit->long_sha);
	fprintf(stream, "From: %s <%s>\n", commit->author, commit->email);
	fprintf(stream, "Date: %s\n", commit->date);
	fprintf(stream, "Subject: %s\n\n", commit->message);

	for (size_t i = 0; i < list.diffs_size; ++i) {
		gitlab_make_commit_diff(commit, &list.diffs[i],
		                        prev_commit_sha, stream);
	}

	fprintf(stream, "--\n2.42.2\n\n\n");

	gitlab_free_diffs(&list);

err_fetch_diffs:

	return rc;
}

int
gitlab_mr_get_patch(struct gcli_ctx *ctx, FILE *stream, char const *owner,
                    char const *reponame, gcli_id mr_number)
{
	int rc = 0;
	char *e_owner, *e_repo;
	struct gcli_pull pull = {0};
	struct gcli_commit_list commits = {0};
	char const *prev_commit_sha;
	char *base_sha_short;

	rc = gitlab_get_pull(ctx, owner, reponame, mr_number, &pull);
	if (rc < 0)
		goto err_get_pull;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(reponame);

	rc = gitlab_get_pull_commits(ctx, owner, reponame, mr_number, &commits);
	if (rc < 0)
		goto err_get_commit_list;

	base_sha_short = sn_strndup(pull.base_sha, 8);
	prev_commit_sha = base_sha_short;
	for (size_t i = commits.commits_size; i > 0; --i) {
		rc = gitlab_make_commit_patch(ctx, stream, e_owner, e_repo,
		                              prev_commit_sha,
		                              &commits.commits[i - 1]);
		if (rc < 0)
			goto err_make_commit_patch;

		prev_commit_sha = commits.commits[i - 1].sha;
	}

err_make_commit_patch:
	free(base_sha_short);
	gcli_commits_free(&commits);

err_get_commit_list:
	free(e_owner);
	free(e_repo);

err_get_pull:
	return rc;
}

static int
gitlab_mr_get_diff_versions(struct gcli_ctx *ctx, char const *const e_owner,
                            char const *const e_repo, gcli_id const mr_number,
                            struct gitlab_mr_version_list *const out)
{
	char *url;
	struct gcli_fetch_list_ctx fl_ctx = {
		.listp = &out->versions,
		.sizep = &out->versions_size,
		.max = -1,
		.parse = (parsefn)parse_gitlab_mr_version_list,
	};

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/versions",
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_number);

	return gcli_fetch_list(ctx, url, &fl_ctx);
}

static int
gitlab_mr_get_diff_version(struct gcli_ctx *ctx, char const *const e_owner,
                           char const *const e_repo, gcli_id const mr_number,
                           gcli_id const version_id,
                           struct gitlab_diff_list *const out)
{
	char *url;
	int rc;
	struct gcli_fetch_buffer buffer = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/versions/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_number,
	                  version_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_gitlab_mr_version_diffs(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	free(buffer.data);

	return rc;
}

int
gitlab_mr_get_diff(struct gcli_ctx *ctx, FILE *stream, char const *owner,
                   char const *repo, gcli_id mr_number)
{
	char *e_owner, *e_repo;
	int rc;
	struct gitlab_diff_list diff_list = {0};
	struct gitlab_mr_version_list version_list = {0};
	struct gitlab_mr_version const *version;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	/* Grab a list of diff versions available for this MR.
	 * The list is sorted numerically decending. Thus we need
	 * to just grab the very first version in the array and use
	 * it. */
	rc = gitlab_mr_get_diff_versions(ctx, e_owner, e_repo, mr_number, &version_list);
	if (rc < 0)
		goto err_get_mr_diff_version;

	if (!version_list.versions_size) {
		rc = gcli_error(ctx, "no diffs available for the merge request");
		goto err_get_mr_diff_version;
	}

	version = &version_list.versions[0];

	rc = gitlab_mr_get_diff_version(ctx, e_owner, e_repo, mr_number,
	                                version->id, &diff_list);

	if (rc < 0)
		goto err_get_diffs;

	fprintf(stream, "GCLI: Below is metadata for this diff. Do not remove or alter\n");
	fprintf(stream, "GCLI: in case you're using this for a review.\n");
	fprintf(stream, "GCLI: base_sha %s\n", version->base_commit);
	fprintf(stream, "GCLI: start_sha %s\n", version->start_commit);
	fprintf(stream, "GCLI: head_sha %s\n", version->head_commit);

	for (size_t i = 0; i < diff_list.diffs_size; ++i) {
		struct gitlab_diff const *const diff = &diff_list.diffs[i];

		fprintf(stream, "diff --git a/%s b/%s\n", diff->old_path, diff->new_path);
		if (diff->new_file) {
			fprintf(stream, "new file mode %s\n", diff->b_mode);
			fprintf(stream, "index 0000000..%s\n", version->head_commit);
		} else {
			fprintf(stream, "index %s..%s %s\n",
			        version->base_commit, version->head_commit,
			        diff->b_mode);
		}

		fprintf(stream, "--- %s%s\n",
		        diff->new_file ? "" : "a/",
		        diff->new_file ? "/dev/null" : diff->old_path);
		fprintf(stream, "+++ %s%s\n",
		        diff->deleted_file ? "" : "b/",
		        diff->deleted_file ? "/dev/null" : diff->new_path);
		fputs(diff->diff, stream);
	}

	gitlab_free_diffs(&diff_list);

err_get_diffs:
	gitlab_mr_version_list_free(&version_list);


err_get_mr_diff_version:
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_mr_set_automerge(struct gcli_ctx *const ctx, char const *const owner,
                        char const *const repo, gcli_id const mr_number)
{
	char *url, *e_owner, *e_repo;
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/merge"
	                  "?merge_when_pipeline_succeeds=true",
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_number);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_mr_merge(struct gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const mr_number, enum gcli_merge_flags const flags)
{
	struct gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char const *data = "{}";
	bool const squash = flags & GCLI_PULL_MERGE_SQUASH;
	bool const delete_source = flags & GCLI_PULL_MERGE_DELETEHEAD;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* PUT /projects/:id/merge_requests/:merge_request_iid/merge */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/merge"
	                  "?squash=%s"
	                  "&should_remove_source_branch=%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, mr_number,
	                  squash ? "true" : "false",
	                  delete_source ? "true" : "false");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, &buffer);

	gcli_fetch_buffer_free(&buffer);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_get_pull(struct gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const pr_number, struct gcli_pull *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, pr_number);
	free(e_owner);
	free(e_repo);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_mr(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
gitlab_get_pull_commits(struct gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const pr_number, struct gcli_commit_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->commits,
		.sizep = &out->commits_size,
		.max = -1,
		.parse = (parsefn)(parse_gitlab_commits),
	};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid/commits */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/commits",
	                  gcli_get_apibase(ctx), e_owner, e_repo, pr_number);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

static int
gitlab_mr_patch_state(struct gcli_ctx *const ctx, char const *const owner,
                      char const *const repo, gcli_id const mr,
                      char const *const new_state)
{
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "state_event");
		gcli_jsongen_string(&gen, new_state);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
gitlab_mr_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const mr)
{
	return gitlab_mr_patch_state(ctx, owner, repo, mr, "close");
}

int
gitlab_mr_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const mr)
{
	return gitlab_mr_patch_state(ctx, owner, repo, mr, "reopen");
}

/* This routine is a workaround for a Gitlab bug:
 *
 * https://gitlab.com/gitlab-org/gitlab/-/issues/353984
 *
 * This is a race condition because something in the creation of a merge request
 * is being handled asynchronously. See the above link for more details.
 *
 * TL;DR: We need to wait until the »merge_status« field of the MR is set to
 * »can_be_merged«. This is indicated by the mergable field becoming true. */
static int
gitlab_mr_wait_until_mergeable(struct gcli_ctx *ctx, char const *const e_owner,
                               char const *const e_repo, gcli_id const mr_id)
{
	char *url;
	int rc = 0;
	struct timespec const ts = { .tv_sec = 1, .tv_nsec = 0 };

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_id);

	for (;;) {
		bool is_mergeable;
		struct gcli_fetch_buffer buffer = {0};
		struct json_stream stream = {0};
		struct gcli_pull pull = {0};

		rc = gcli_fetch(ctx, url, NULL, &buffer);
		if (rc < 0)
			break;

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_gitlab_mr(ctx, &stream, &pull);
		json_close(&stream);

		/* FIXME: this doesn't quite cut it when the PR has no commits in it.
		 * In that case this will turn into an infinite loop. */
		is_mergeable = pull.mergeable;

		gcli_pull_free(&pull);
	gcli_fetch_buffer_free(&buffer);

		if (is_mergeable)
			break;

		/* sort of a hack: wait for a second until the next request goes out */
		nanosleep(&ts, NULL);
	}

	free(url);

	return rc;
}

int
gitlab_perform_submit_mr(struct gcli_ctx *ctx, struct gcli_submit_pull_options *opts)
{
	/* Note: this doesn't really allow merging into repos with
	 * different names. We need to figure out a way to make this
	 * better for both github and gitlab. */
	char *source_branch = NULL, *source_owner = NULL, *payload = NULL,
	     *e_owner = NULL, *e_repo = NULL, *url = NULL;
	char const *target_branch = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	struct gcli_repo target = {0};

	target_branch = opts->to;
	source_owner = strdup(opts->from);
	source_branch = strchr(source_owner, ':');
	if (source_branch == NULL)
		return gcli_error(ctx, "bad merge request source: expected 'owner:branch'");

	*source_branch++ = '\0';

	/* Figure out the project id */
	rc = gitlab_get_repo(ctx, opts->owner, opts->repo, &target);
	if (rc < 0)
		return rc;

	/* generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "source_branch");
		gcli_jsongen_string(&gen, source_branch);

		gcli_jsongen_objmember(&gen, "target_branch");
		gcli_jsongen_string(&gen, target_branch);

		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, opts->title);

		/* description is optional and will be NULL if unset */
		if (opts->body) {
			gcli_jsongen_objmember(&gen, "description");
			gcli_jsongen_string(&gen, opts->body);
		}

		gcli_jsongen_objmember(&gen, "target_project_id");
		gcli_jsongen_number(&gen, target.id);

		/* Labels if any */
		if (opts->labels_size) {
			gcli_jsongen_objmember(&gen, "labels");

			gcli_jsongen_begin_array(&gen);
			for (size_t i = 0; i < opts->labels_size; ++i)
				gcli_jsongen_string(&gen, opts->labels[i]);
			gcli_jsongen_end_array(&gen);
		}

		/* Reviewers if any */
		if (opts->reviewers_size) {
			gcli_jsongen_objmember(&gen, "reviewer_ids");

			gcli_jsongen_begin_array(&gen);
			for (size_t i = 0; i < opts->reviewers_size; ++i) {
				int uid;

				uid = gitlab_user_id(ctx, opts->reviewers[i]);
				if (uid < 0)
					return uid;

				gcli_jsongen_number(&gen, uid);
			}
			gcli_jsongen_end_array(&gen);
		}
	}
	gcli_jsongen_end_object(&gen);
	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);
	gcli_repo_free(&target);

	/* generate url */
	e_owner = gcli_urlencode(source_owner);
	e_repo = gcli_urlencode(opts->repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);

	/* if that succeeded and the user wants automerge, parse the result and
	 * set the automerge flag */
	if (rc == 0 && opts->automerge) {
		struct json_stream stream = {0};
		struct gcli_pull pull = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_gitlab_mr(ctx, &stream, &pull);
		json_close(&stream);

		if (rc < 0)
			goto out;

		rc = gitlab_mr_wait_until_mergeable(ctx, e_owner, e_repo, pull.number);
		if (rc < 0)
			goto out;

		rc = gitlab_mr_set_automerge(ctx, opts->owner, opts->repo, pull.number);

	out:
		gcli_pull_free(&pull);
	}

	/* cleanup */
	gcli_fetch_buffer_free(&buffer);
	free(e_owner);
	free(e_repo);
	free(source_owner);
	free(payload);
	free(url);

	return rc;
}

static int
gitlab_mr_update_labels(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id const mr,
                        char const *const labels[], size_t const labels_size,
                        char const *const update_action)
{
	char *url  = NULL, *payload = NULL, *list = NULL, *e_owner = NULL,
	     *e_repo = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate payload */
	list = sn_join_with(labels, labels_size, ",");
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, update_action);
		gcli_jsongen_string(&gen, list);
	}
	gcli_jsongen_end_object(&gen);
	payload = gcli_jsongen_to_string(&gen);

	gcli_jsongen_free(&gen);
	free(list);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
gitlab_mr_add_labels(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id const mr, char const *const labels[],
                     size_t const labels_size)
{
	return gitlab_mr_update_labels(ctx, owner, repo, mr, labels, labels_size,
	                               "add_labels");
}

int
gitlab_mr_remove_labels(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id const mr,
                        char const *const labels[], size_t const labels_size)
{
	return gitlab_mr_update_labels(ctx, owner, repo, mr, labels, labels_size,
	                               "remove_labels");
}

int
gitlab_mr_set_milestone(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id mr, gcli_id milestone_id)
{
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate Payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "milestone_id");
		gcli_jsongen_id(&gen, milestone_id);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
gitlab_mr_clear_milestone(struct gcli_ctx *ctx, char const *owner,
                          char const *repo, gcli_id const mr)
{
	/* GitLab's REST API docs state:
	 *
	 * The global ID of a milestone to assign the merge request
	 * to. Set to 0 or provide an empty value to unassign a
	 * milestone. */
	return gitlab_mr_set_milestone(ctx, owner, repo, mr, 0);
}

/* Helper function to fetch the list of user ids that are reviewers
 * of a merge requests. */
static int
gitlab_mr_get_reviewers(struct gcli_ctx *ctx, char const *e_owner,
                        char const *e_repo, gcli_id const mr,
                        struct gitlab_reviewer_id_list *const out)
{
	char *url;
	int rc;
	struct gcli_fetch_buffer buffer = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_reviewer_ids(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

static void
gitlab_reviewer_list_free(struct gitlab_reviewer_id_list *const list)
{
	free(list->reviewers);
	list->reviewers = NULL;
	list->reviewers_size = 0;
}

int
gitlab_mr_add_reviewer(struct gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id mr_number, char const *username)
{
	char *url, *e_owner, *e_repo, *payload;
	int uid, rc = 0;
	struct gitlab_reviewer_id_list list = {0};
	struct gcli_jsongen gen = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	/* Fetch list of already existing reviewers */
	rc = gitlab_mr_get_reviewers(ctx, e_owner, e_repo, mr_number, &list);
	if (rc < 0)
		goto bail_get_reviewers;

	/* Resolve user id from user name */
	uid = gitlab_user_id(ctx, username);
	if (uid < 0)
		goto bail_resolve_user_id;

	/* Start generating payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "reviewer_ids");

		gcli_jsongen_begin_array(&gen);
		{
			for (size_t i = 0; i < list.reviewers_size; ++i)
				gcli_jsongen_number(&gen, list.reviewers[i]);

			/* Push new user id into list of user ids */
			gcli_jsongen_number(&gen, uid);
		}
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);


	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* generate URL */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_number);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

bail_resolve_user_id:
	gitlab_reviewer_list_free(&list);

bail_get_reviewers:
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_mr_set_title(struct gcli_ctx *ctx, char const *const owner,
                    char const *const repo, gcli_id const id,
                    char const *const new_title)
{
	char *url, *e_owner, *e_repo, *payload;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate url
	 *
	 * PUT /projects/:id/merge_requests/:merge_request_iid */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, id);
	free(e_owner);
	free(e_repo);

	/* Generate payload */
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
	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	/* clean up */
	free(url);
	free(payload);

	return rc;
}

/* Compute the SHA1 message digest of the given input string. Returns
 * NULL on failure or the hexadecimal representation of the digest on
 * success */
static char *
digest_sha1(char const *const string)
{
	int success = 0;
	unsigned char hash_data[20] = {0};
	unsigned int hash_data_len = sizeof(hash_data);

	size_t const result_size = 41;
	char *result = calloc(result_size, 1);

	success = EVP_Digest(string, strlen(string), hash_data, &hash_data_len,
	                     EVP_sha1(), NULL);
	if (!success)
		return NULL;

	for (size_t i = 0; i < sizeof(hash_data); ++i) {
		snprintf(result + 2*i, result_size - 2*i, "%02x", hash_data[i]);
	}

	return result;
}

static int
line_code(struct gcli_ctx *ctx, struct gcli_jsongen *const gen,
          char const *filename, int const old, int const new)
{
	char tmp[128] = {0};
	char *sha_digest;

	sha_digest = digest_sha1(filename);
	if (!sha_digest)
		return gcli_error(ctx, "failed to produce SHA1 digest of filename");

	snprintf(tmp, sizeof(tmp), "%s_%d_%d", sha_digest, old, new);
	gcli_jsongen_string(gen, tmp);

	free(sha_digest);

	return 0;
}

static int
post_diff_comment(struct gcli_ctx *ctx,
                  struct gcli_pull_create_review_details const *details,
                  struct gcli_diff_comment const *comment)
{
	char *e_owner, *e_repo;
	char *url, *payload;
	char const *base_sha, *start_sha, *head_sha;
	int rc = 0;
	struct gcli_jsongen gen = {0};

	if ((base_sha = gcli_pull_get_meta_by_key(details, "base_sha")) == NULL)
		return gcli_error(ctx, "no base_sha in meta");

	if ((start_sha = gcli_pull_get_meta_by_key(details, "start_sha")) == NULL)
		return gcli_error(ctx, "no start_sha in meta");

	if ((head_sha = gcli_pull_get_meta_by_key(details, "head_sha")) == NULL)
		return gcli_error(ctx, "no head_sha in meta");

	e_owner = gcli_urlencode(details->owner);
	e_repo  = gcli_urlencode(details->repo);


	/* /projects/:id/merge_requests/:merge_request_iid/discussions */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/discussions",
	                  gcli_get_apibase(ctx), e_owner, e_repo,
	                  details->pull_id);

	/* Generate payload */
	if (gcli_jsongen_init(&gen) < 0)
		goto err_jsongen_init;

	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "body");
		gcli_jsongen_string(&gen, comment->comment);

		gcli_jsongen_objmember(&gen, "commit_id");
		gcli_jsongen_string(&gen, comment->commit_hash);

		gcli_jsongen_objmember(&gen, "position");
		gcli_jsongen_begin_object(&gen);
		{
			gcli_jsongen_objmember(&gen, "position_type");
			gcli_jsongen_string(&gen, "text");

			gcli_jsongen_objmember(&gen, "base_sha");
			gcli_jsongen_string(&gen, base_sha);

			gcli_jsongen_objmember(&gen, "start_sha");
			gcli_jsongen_string(&gen, start_sha);

			gcli_jsongen_objmember(&gen, "head_sha");
			gcli_jsongen_string(&gen, head_sha);

			gcli_jsongen_objmember(&gen, "new_path");
			gcli_jsongen_string(&gen, comment->after.filename);

			gcli_jsongen_objmember(&gen, "old_path");
			gcli_jsongen_string(&gen, comment->before.filename);

			gcli_jsongen_objmember(&gen, "new_line");
			gcli_jsongen_number(&gen, comment->after.start_row);

			gcli_jsongen_objmember(&gen, "line_range");
			gcli_jsongen_begin_object(&gen);
			{

				gcli_jsongen_objmember(&gen, "start");
				gcli_jsongen_begin_object(&gen);
				{
					gcli_jsongen_objmember(&gen, "type");
					gcli_jsongen_string(&gen, comment->start_is_in_new ? "new" : "old");

					gcli_jsongen_objmember(&gen, "line_code");
					line_code(ctx, &gen, comment->after.filename,
					          comment->before.start_row, comment->after.start_row);
				}
				gcli_jsongen_end_object(&gen);

				gcli_jsongen_objmember(&gen, "end");
				gcli_jsongen_begin_object(&gen);
				{
					gcli_jsongen_objmember(&gen, "type");
					gcli_jsongen_string(&gen, comment->end_is_in_new ? "new" : "old");

					gcli_jsongen_objmember(&gen, "line_code");
					line_code(ctx, &gen, comment->after.filename,
					          comment->before.end_row, comment->after.end_row);
				}
				gcli_jsongen_end_object(&gen);
			}
			gcli_jsongen_end_object(&gen);
		}
		gcli_jsongen_end_object(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(payload);

err_jsongen_init:
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
gitlab_mr_create_review(struct gcli_ctx *ctx,
                        struct gcli_pull_create_review_details const *const details)
{
	int rc;
	struct gcli_diff_comment const *comment;

	TAILQ_FOREACH(comment, &details->comments, next) {
		rc = post_diff_comment(ctx, details, comment);
		if (rc < 0)
			return rc;
	}

	/* Abort on failure */
	if (rc < 0)
		return rc;

	/* Check whether we wish to submit a general comment */
	if (details->body && *details->body) {
		struct gcli_submit_comment_opts opts = {
			.owner = details->owner,
			.repo = details->repo,
			.target_id = details->pull_id,
			.target_type = PR_COMMENT,
			.message = details->body,
		};

		rc = gitlab_perform_submit_comment(ctx, &opts);
		if (rc < 0)
			return rc;
	}

	/* Check whether to approve or unapprove the MR */
	switch (details->review_state) {
	case GCLI_REVIEW_ACCEPT_CHANGES:
		rc = gitlab_mr_approve(ctx, details->owner, details->repo,
		                       details->pull_id);
		break;
	case GCLI_REVIEW_REQUEST_CHANGES:
		rc = gitlab_mr_unapprove(ctx, details->owner, details->repo,
		                         details->pull_id);
		break;
	default:
		/* commenting only implies no change to the merge request */
		break;
	}

	return rc;
}

void
gitlab_mr_version_free(struct gitlab_mr_version *version)
{
	free(version->base_commit);
	free(version->start_commit);
	free(version->head_commit);
}

void
gitlab_mr_version_list_free(struct gitlab_mr_version_list *list)
{
	for (size_t i = 0; i < list->versions_size; ++i) {
		gitlab_mr_version_free(&list->versions[i]);
	}

	free(list->versions);

	list->versions = NULL;
	list->versions_size = 0;
}

static int
gitlab_mr_request_update_approval(struct gcli_ctx *ctx, char const *const owner,
                                  char const *const repo, gcli_id const mr,
                                  char const *const action)
{
	int rc = 0;
	char *url = NULL, *e_owner = NULL, *e_repo = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/%s",
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr, action);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, "{}", NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_mr_approve(struct gcli_ctx *ctx, char const *const owner,
                  char const *const repo, gcli_id const mr)
{
	return gitlab_mr_request_update_approval(ctx, owner, repo, mr, "approve");
}

int
gitlab_mr_unapprove(struct gcli_ctx *ctx, char const *const owner,
                    char const *const repo, gcli_id const mr)
{
	return gitlab_mr_request_update_approval(ctx, owner, repo, mr, "unapprove");
}
