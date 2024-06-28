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

#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/milestones.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

#include <templates/github/issues.h>

/* TODO: Remove this function once we use linked lists for storing
 *       issues.
 *
 * This is an ugly hack caused by the sillyness of the Github API that
 * treats Pull Requests as issues and reports them to us when we
 * request issues. This function nukes them from the list, readjusts
 * the allocation size and fixes the reported list size. */
static void
github_hack_fixup_issues_that_are_actually_pulls(struct gcli_issue **list, size_t *size,
                                                 void *_data)
{
	(void) _data;

	for (size_t i = *size; i > 0; --i) {
		if ((*list)[i-1].is_pr) {
			struct gcli_issue *l = *list;
			/*  len = 7, i = 5, to move = 7 - 5 = 2
			 *   0   1   2   3   4   5   6
			 * | x | x | x | x | X | x | x | */
			gcli_issue_free(&l[i-1]);
			memmove(&l[i-1], &l[i],
			        sizeof(*l) * (*size - i));
			*list = realloc(l, (--(*size)) * sizeof(*l));
		}
	}
}

int
github_fetch_issues(struct gcli_ctx *ctx, char *url, int const max,
                    struct gcli_issue_list *const out)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->issues,
		.sizep = &out->issues_size,
		.parse = (parsefn)(parse_github_issues),
		.filter = (filterfn)(github_hack_fixup_issues_that_are_actually_pulls),
		.max = max,
	};

	return gcli_fetch_list(ctx, url, &fl);
}

static int
get_milestone_id(struct gcli_ctx *ctx, char const *owner, char const *repo,
                 char const *milestone_name, gcli_id *out)
{
	int rc = 0;
	struct gcli_milestone_list list = {0};

	rc = github_get_milestones(ctx, owner, repo, -1, &list);
	if (rc < 0)
		return rc;

	rc = gcli_error(ctx, "%s: no such milestone", milestone_name);

	for (size_t i = 0; i < list.milestones_size; ++i) {
		if (strcmp(list.milestones[i].title, milestone_name) == 0) {
			*out = list.milestones[i].id;
			rc = 0;
			break;
		}
	}

	gcli_free_milestones(&list);

	return rc;
}

static int
parse_github_milestone(struct gcli_ctx *ctx, char const *owner,
                       char const *repo, char const *milestone, gcli_id *out)
{
	char *endptr = NULL;
	size_t const m_len = strlen(milestone);

	/* first try parsing as a milestone ID, if it isn't one,
	 * go looking for a similarly named milestone */
	*out = strtoull(milestone, &endptr, 10);
	if (endptr == milestone + m_len)
		return 0;

	return get_milestone_id(ctx, owner, repo, milestone, out);
}

/* Search issues with a search term */
static int
search_issues(struct gcli_ctx *ctx, char const *owner, char const *repo,
              struct gcli_issue_fetch_details const *details, int const max,
              struct gcli_issue_list *const out)
{
	char *url = NULL, *query_string = NULL, *e_query_string = NULL,
	     *milestone = NULL, *author = NULL, *label = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	(void) max;

	if (details->milestone)
		milestone = sn_asprintf("milestone:%s", details->milestone);

	if (details->author)
		author = sn_asprintf("author:%s", details->author);

	if (details->label)
		label = sn_asprintf("label:%s", details->label);

	query_string = sn_asprintf("repo:%s/%s is:issue%s %s %s %s %s", owner, repo,
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
	rc = parse_github_issue_search_result(ctx, &stream, out);

	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

/* Optimised routine for issues without a search term */
static int
get_issues(struct gcli_ctx *ctx, char const *owner, char const *repo,
           struct gcli_issue_fetch_details const *details, int const max,
           struct gcli_issue_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;
	char *e_label = NULL;
	char *e_milestone = NULL;

	if (details->milestone) {
		gcli_id milestone_id;
		int rc;

		rc = parse_github_milestone(ctx, owner, repo, details->milestone, &milestone_id);
		if (rc < 0)
			return rc;

		e_milestone = sn_asprintf("&milestone=%"PRIid, milestone_id);
	}

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("&creator=%s", tmp);
		free(tmp);
	}

	if (details->label) {
		char *tmp = gcli_urlencode(details->label);
		e_label = sn_asprintf("&labels=%s", tmp);
		free(tmp);
	}

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues?state=%s%s%s%s",
		gcli_get_apibase(ctx),
		e_owner, e_repo,
		details->all ? "all" : "open",
		e_author ? e_author : "",
		e_label ? e_label : "",
		e_milestone ? e_milestone : "");

	free(e_milestone);
	free(e_author);
	free(e_label);
	free(e_owner);
	free(e_repo);

	return github_fetch_issues(ctx, url, max, out);
}

int
github_issues_search(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     struct gcli_issue_fetch_details const *details,
                     int const max, struct gcli_issue_list *const out)
{

	if (details->search_term)
		return search_issues(ctx, owner, repo, details, max, out);
	else
		return get_issues(ctx, owner, repo, details, max, out);
}

int
github_fetch_issue(struct gcli_ctx *const ctx, char *const url,
                   struct gcli_issue *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream parser = {0};
	int rc = 0;

	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&parser, buffer.data, buffer.length);
		json_set_streaming(&parser, true);
		parse_github_issue(ctx, &parser, out);
		json_close(&parser);
	}

	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_get_issue_summary(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id const issue_number,
                         struct gcli_issue *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo,
	                  issue_number);

	free(e_owner);
	free(e_repo);

	rc = github_fetch_issue(ctx, url, out);
	free(url);

	return rc;
}

static int
github_issue_patch_state(struct gcli_ctx *ctx, char const *const owner,
                         char const *const repo, gcli_id const issue,
                         char const *const state)
{
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);
	payload = sn_asprintf("{ \"state\": \"%s\"}", state);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
github_issue_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   gcli_id const issue)
{
	return github_issue_patch_state(ctx, owner, repo, issue, "closed");
}

int
github_issue_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue)
{
	return github_issue_patch_state(ctx, owner, repo, issue, "open");
}

int
github_perform_submit_issue(struct gcli_ctx *const ctx,
                            struct gcli_submit_issue_options *const opts,
                            struct gcli_issue *const out)
{
	char *e_owner = NULL, *e_repo = NULL, *payload = NULL, *url = NULL;
	struct gcli_jsongen gen = {0};
	struct gcli_fetch_buffer buffer = {0}, *_buffer = NULL;
	int rc = 0;

	/* Generate Payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, opts->title);

		/* Body can be omitted and is NULL in that case */
		if (opts->body) {
			gcli_jsongen_objmember(&gen, "body");
			gcli_jsongen_string(&gen, opts->body);
		}
	}
	gcli_jsongen_begin_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(opts->owner);
	e_repo = gcli_urlencode(opts->repo);

	url = sn_asprintf("%s/repos/%s/%s/issues", gcli_get_apibase(ctx), e_owner,
	                  e_repo);

	free(e_owner);
	free(e_repo);

	/* only read the resulting data if the issue data has been requested */
	if (out)
		_buffer = &buffer;

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, _buffer);
	if (out && rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_github_issue(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(payload);
	free(url);

	return rc;
}

int
github_issue_assign(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue_number, char const *assignee)
{
	struct gcli_jsongen gen = {0};
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	int rc = 0;

	/* Generate Payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "assignees");
		gcli_jsongen_begin_array(&gen);
		gcli_jsongen_string(&gen, assignee);
		gcli_jsongen_end_object(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"/assignees",
	                  gcli_get_apibase(ctx), e_owner, e_repo, issue_number);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
github_issue_add_labels(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id const issue,
                        char const *const labels[], size_t const labels_size)
{
	char *url = NULL;
	char *data = NULL;
	char *list = NULL;
	int rc = 0;

	assert(labels_size > 0);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"/labels",
	                  gcli_get_apibase(ctx), owner, repo, issue);

	list = sn_join_with(labels, labels_size, "\",\"");
	data = sn_asprintf("{ \"labels\": [\"%s\"]}", list);

	rc = gcli_fetch_with_method(ctx, "POST", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
github_issue_remove_labels(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id const issue,
                           char const *const labels[], size_t const labels_size)
{
	char *url = NULL;
	char *e_label = NULL;
	int rc = 0;

	if (labels_size != 1) {
		return gcli_error(ctx, "GitHub only supports removing labels from "
		                  "issues one by one.");
	}

	e_label = gcli_urlencode(labels[0]);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"/labels/%s",
	                  gcli_get_apibase(ctx), owner, repo, issue, e_label);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_label);

	return rc;
}

int
github_issue_set_milestone(struct gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const issue,
                           gcli_id const milestone)
{
	char *url, *e_owner, *e_repo, *body;
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid,
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);

	body = sn_asprintf("{ \"milestone\": %"PRIid" }", milestone);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, body, NULL, NULL);

	free(body);
	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
github_issue_clear_milestone(struct gcli_ctx *ctx, char const *const owner,
                             char const *const repo, gcli_id const issue)
{
	char *url, *e_owner, *e_repo;
	char const *payload;
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid,
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);

	payload = "{ \"milestone\": null }";

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
github_issue_set_title(struct gcli_ctx *ctx, char const *const owner,
                       char const *const repo, gcli_id const issue,
                       char const *const new_title)
{
	char *url, *e_owner, *e_repo, *payload;
	struct gcli_jsongen gen = {0};
	int rc;

	/* Generate url */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);

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

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}
