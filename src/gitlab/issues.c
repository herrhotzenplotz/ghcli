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
#include <gcli/gitlab/api.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/issues.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/gitlab/issues.h>

#include <pdjson/pdjson.h>

/** Given the url fetch issues */
int
gitlab_fetch_issues(struct gcli_ctx *ctx, char *url, int const max,
                    struct gcli_issue_list *const out)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->issues,
		.sizep = &out->issues_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_issues),
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_issues_search(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     struct gcli_issue_fetch_details const *details,
                     int const max, struct gcli_issue_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;
	char *e_labels = NULL;
	char *e_milestone = NULL;
	char *e_search = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("%cauthor_username=%s",
		                       details->all ? '?' : '&',
		                       tmp);
		free(tmp);
	}

	if (details->label) {
		char *tmp = gcli_urlencode(details->label);
		int const should_do_qmark = details->all && !details->author;

		e_labels = sn_asprintf("%clabels=%s", should_do_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->milestone) {
		char *tmp = gcli_urlencode(details->milestone);
		int const should_do_qmark = details->all && !details->author &&
		                            !details->label;

		e_milestone = sn_asprintf("%cmilestone=%s", should_do_qmark ? '?' : '&',
		                          tmp);
		free(tmp);
	}

	if (details->search_term) {
		char *tmp = gcli_urlencode(details->search_term);
		int const should_do_qmark = details->all && !details->author &&
		                            !details->label && !details->milestone;
		e_search = sn_asprintf("%csearch=%s", should_do_qmark ? '?': '&', tmp);
		free(tmp);
	}

	url = sn_asprintf("%s/projects/%s%%2F%s/issues%s%s%s%s%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, details->all ? "" : "?state=opened",
	                  e_author ? e_author : "", e_labels ? e_labels : "",
	                  e_milestone ? e_milestone : "",
	                  e_search ? e_search : "");

 	free(e_milestone);
 	free(e_author);
	free(e_labels);
	free(e_owner);
	free(e_repo);

	return gitlab_fetch_issues(ctx, url, max, out);
}

int
gitlab_get_issue_summary(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, gcli_id const issue_number,
                         struct gcli_issue *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream parser = {0};
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue_number);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&parser, buffer.data, buffer.length);
		json_set_streaming(&parser, true);
		parse_gitlab_issue(ctx, &parser, out);
		json_close(&parser);
	}

	free(url);
	free(e_owner);
	free(e_repo);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

static int
gitlab_issue_patch_state(struct gcli_ctx *const ctx, char const *const owner,
                         char const *const repo, gcli_id const issue,
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

	url  = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid,
	                   gcli_get_apibase(ctx), e_owner, e_repo, issue);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}

int
gitlab_issue_close(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   gcli_id const issue)
{
	return gitlab_issue_patch_state(ctx, owner, repo, issue, "close");
}

int
gitlab_issue_reopen(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue)
{
	return gitlab_issue_patch_state(ctx, owner, repo, issue, "reopen");
}

int
gitlab_perform_submit_issue(struct gcli_ctx *const ctx,
                            struct gcli_submit_issue_options *const opts,
                            struct gcli_issue *const out)
{
	char *e_owner = NULL, *e_repo = NULL, *url = NULL, *payload = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0}, *_buffer = NULL;
	struct gcli_jsongen gen = {0};

	e_owner = gcli_urlencode(opts->owner);
	e_repo = gcli_urlencode(opts->repo);

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, opts->title);

		/* The body may be NULL if empty. In this case we can omit the
		 * body / description as it is not required by the API */
		if (opts->body) {
			gcli_jsongen_objmember(&gen, "description");
			gcli_jsongen_string(&gen, opts->body);
		}
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	if (out)
		_buffer = &buffer;

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, _buffer);
	if (rc == 0 && out) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_gitlab_issue(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(payload);
	free(url);

	return rc;
}

int
gitlab_issue_assign(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue_number, char const *assignee)
{
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	struct gcli_jsongen gen = {0};
	int assignee_uid = -1;
	int rc = 0;

	assignee_uid = gitlab_user_id(ctx, assignee);
	if (assignee_uid < 0)
		return assignee_uid;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "assignee_ids");
		gcli_jsongen_begin_array(&gen);
		gcli_jsongen_number(&gen, assignee_uid);
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue_number);
	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

static int
gitlab_issues_update_labels(struct gcli_ctx *const ctx, char const *const owner,
                            char const *const repo, gcli_id const issue,
                            char const *const labels[], size_t const labels_size,
                            char const *const what)
{
	char *url = NULL, *payload = NULL, *label_list = NULL, *e_owner = NULL,
	     *e_repo = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate payload. For some reason Gitlab expects us to put a
	 * comma-separated list of issues into a JSON string. Figures...*/
	label_list = sn_join_with(labels, labels_size, ",");

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, what);
		gcli_jsongen_string(&gen, label_list);
	}
	gcli_jsongen_end_object(&gen);

	free(label_list);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}

int
gitlab_issue_add_labels(struct gcli_ctx *ctx, char const *owner,
                        char const *repo, gcli_id const issue,
                        char const *const labels[], size_t const labels_size)
{
	return gitlab_issues_update_labels(ctx, owner, repo, issue, labels,
	                                   labels_size, "add_labels");
}

int
gitlab_issue_remove_labels(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id const issue,
                           char const *const labels[], size_t const labels_size)
{
	return gitlab_issues_update_labels(ctx, owner, repo, issue, labels,
	                                   labels_size, "remove_labels");
}

int
gitlab_issue_set_milestone(struct gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const issue,
                           gcli_id const milestone)
{
	char *url, *e_owner, *e_repo;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid"?milestone_id=%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, issue, milestone);

	rc = gcli_fetch_with_method(ctx, "PUT", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_issue_clear_milestone(struct gcli_ctx *ctx, char const *const owner,
                             char const *const repo, gcli_id const issue)
{
	char *url, *e_owner, *e_repo;
	char const *payload;
	int rc;

	/* The Gitlab API says:
	 *
	 *    milestone_id: The global ID of a milestone to assign the
	 *    issue to. Set to 0 or provide an empty value to unassign a
	 *    milestone.
	 *
	 * However, the documentation is plain wrong and trying to set it
	 * to zero does absolutely nothing. What do you expect from an
	 * enterprise quality product?! Certainly not this kind of spanish
	 * inquisition. Fear and surprise. That's the Gitlab API in a
	 * nutshell.*/

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);
	payload = "{ \"milestone_id\": null }";

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_issue_set_title(struct gcli_ctx *ctx, char const *owner,
                       char const *repo, gcli_id issue,
                       char const *const new_title)
{
	char *url, *e_owner, *e_repo, *payload;
	struct gcli_jsongen gen = {0};
	int rc;

	/* Generate url */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, issue);
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

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}
