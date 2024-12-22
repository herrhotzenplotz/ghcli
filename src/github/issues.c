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
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/milestones.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <templates/github/issues.h>

#include <assert.h>
#include <stdarg.h>

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
github_issue_make_url(struct gcli_ctx *const ctx,
                      struct gcli_path const *const path,
                      char **const url, char const *const fmt, ...)
{
	int rc = 0;
	va_list vp;
	char *suffix = NULL;

	va_start(vp, fmt);
	suffix = sn_vasprintf(fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   path->data.as_default.id, suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind");
	} break;
	}

	free(suffix);

	return rc;
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
get_milestone_id(struct gcli_ctx *ctx, struct gcli_path const *const path,
                 char const *milestone_name, gcli_id *out)
{
	int rc = 0;
	struct gcli_milestone_list list = {0};

	rc = github_get_milestones(ctx, path, -1, &list);
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
parse_github_milestone(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       char const *milestone, gcli_id *out)
{
	char *endptr = NULL;
	size_t const m_len = strlen(milestone);

	/* first try parsing as a milestone ID, if it isn't one,
	 * go looking for a similarly named milestone */
	*out = strtoull(milestone, &endptr, 10);
	if (endptr == milestone + m_len)
		return 0;

	return get_milestone_id(ctx, path, milestone, out);
}

/* Search issues with a search term */
static int
search_issues(struct gcli_ctx *ctx, struct gcli_path const *const path,
              struct gcli_issue_fetch_details const *details, int const max,
              struct gcli_issue_list *const out)
{
	char *url = NULL, *query_string = NULL, *e_query_string = NULL,
	     *milestone = NULL, *author = NULL, *label = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	(void) max;

	/* Search only works with default paths */
	if (path->kind != GCLI_PATH_DEFAULT)
		return gcli_error(ctx, "unsupported path kind for issue search");

	/* Encode the various fields */
	if (details->milestone)
		milestone = sn_asprintf("milestone:%s", details->milestone);

	if (details->author)
		author = sn_asprintf("author:%s", details->author);

	if (details->label)
		label = sn_asprintf("label:%s", details->label);

	query_string = sn_asprintf("repo:%s/%s is:issue%s %s %s %s %s",
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
	rc = parse_github_issue_search_result(ctx, &stream, out);

	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

/** Routine for generating a URL for getting issues of a repository given its
 * path. */
static int
github_issues_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       char const *const suffix, char **out)
{
	int rc = 0;

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);
		*out = sn_asprintf("%s/repos/%s/%s/issues%s",
		                   gcli_get_apibase(ctx),
		                   e_owner, e_repo, suffix);
		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*out = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for issue list");
	} break;
	}

	return rc;
}

/* Optimised routine for issues without a search term */
static int
get_issues(struct gcli_ctx *ctx, struct gcli_path const *const path,
           struct gcli_issue_fetch_details const *details, int const max,
           struct gcli_issue_list *const out)
{
	char *url = NULL, *e_author = NULL, *e_label = NULL,
	     *e_milestone = NULL, *suffix = NULL;
	int rc = 0;

	if (details->milestone) {
		gcli_id milestone_id;

		rc = parse_github_milestone(ctx, path, details->milestone, &milestone_id);
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

	suffix = sn_asprintf(
		"?state=%s%s%s%s",
		details->all ? "all" : "open",
		e_author ? e_author : "",
		e_label ? e_label : "",
		e_milestone ? e_milestone : "");

	rc = github_issues_make_url(ctx, path, suffix, &url);

	free(e_milestone);
	free(e_author);
	free(e_label);
	free(suffix);

	if (rc < 0)
		return rc;

	return github_fetch_issues(ctx, url, max, out);
}

int
github_issues_search(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     struct gcli_issue_fetch_details const *details,
                     int const max, struct gcli_issue_list *const out)
{

	if (details->search_term)
		return search_issues(ctx, path, details, max, out);
	else
		return get_issues(ctx, path, details, max, out);
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
github_get_issue_summary(struct gcli_ctx *ctx, struct gcli_path const *const path,
                         struct gcli_issue *const out)
{
	char *url = NULL;
	int rc = 0;

	rc = github_issue_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = github_fetch_issue(ctx, url, out);
	free(url);

	return rc;
}

static int
github_issue_patch_state(struct gcli_ctx *ctx,
                         struct gcli_path const *const path,
                         char const *const state)
{
	char *url = NULL, *payload = NULL;
	int rc = 0;

	rc = github_issue_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	payload = sn_asprintf("{ \"state\": \"%s\"}", state);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}

int
github_issue_close(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return github_issue_patch_state(ctx, path, "closed");
}

int
github_issue_reopen(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return github_issue_patch_state(ctx, path, "open");
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
github_issue_assign(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    char const *assignee)
{
	struct gcli_jsongen gen = {0};
	char *url = NULL, *payload = NULL;
	int rc = 0;

	/* Generate URL */
	rc = github_issue_make_url(ctx, path, &url, "/assignees");
	if (rc < 0)
		return rc;

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

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
github_issue_add_labels(struct gcli_ctx *ctx,
                        struct gcli_path const *const issue_path,
                        char const *const labels[], size_t const labels_size)
{
	char *data = NULL, *url = NULL;
	int rc = 0;
	struct gcli_jsongen gen = {0};

	assert(labels_size > 0);

	rc = github_issue_make_url(ctx, issue_path, &url, "/labels");
	if (rc < 0)
		return rc;

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "labels");
		gcli_jsongen_begin_array(&gen);
		for (size_t i = 0; i < labels_size; ++i) {
			gcli_jsongen_string(&gen, labels[i]);
		}
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	data = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, data, NULL, NULL);

	free(url);
	free(data);

	return rc;
}

int
github_issue_remove_labels(struct gcli_ctx *ctx,
                           struct gcli_path const *const path,
                           char const *const labels[], size_t const labels_size)
{
	char *url = NULL, *e_label = NULL;
	int rc = 0;

	if (labels_size != 1) {
		return gcli_error(ctx, "GitHub only supports removing labels from "
		                  "issues one by one.");
	}

	e_label = gcli_urlencode(labels[0]);

	rc = github_issue_make_url(ctx, path, &url, "/labels/%s", e_label);
	if (rc == 0)
		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_label);

	return rc;
}

int
github_issue_set_milestone(struct gcli_ctx *ctx,
                           struct gcli_path const *const issue_path,
                           gcli_id const milestone)
{
	char *url, *body;
	int rc;

	rc = github_issue_make_url(ctx, issue_path, &url, "");
	if (rc < 0)
		return rc;

	body = sn_asprintf("{ \"milestone\": %"PRIid" }", milestone);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, body, NULL, NULL);

	free(body);
	free(url);

	return rc;
}

int
github_issue_clear_milestone(struct gcli_ctx *ctx,
                             struct gcli_path const *const path)
{
	char *url = NULL;
	char const *payload = NULL;
	int rc;

	rc = github_issue_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	payload = "{ \"milestone\": null }";

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(url);

	return rc;
}

int
github_issue_set_title(struct gcli_ctx *ctx,
                       struct gcli_path const *const issue_path,
                       char const *const new_title)
{
	char *url, *payload;
	struct gcli_jsongen gen = {0};
	int rc;

	/* Generate url */
	rc = github_issue_make_url(ctx, issue_path, &url, "");
	if (rc < 0)
		return rc;

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
