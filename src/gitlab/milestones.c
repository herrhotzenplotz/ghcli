/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/date_time.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/issues.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/milestones.h>
#include <gcli/json_util.h>

#include <templates/gitlab/milestones.h>

#include <pdjson/pdjson.h>

#include <assert.h>
#include <time.h>

int
gitlab_get_milestones(struct gcli_ctx *ctx, char const *owner, char const *repo,
                      int max, struct gcli_milestone_list *const out)
{
	char *url;
	char *e_owner, *e_repo;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->milestones,
		.sizep = &out->milestones_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_milestones),
	};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_milestone(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id const milestone, struct gcli_milestone *const out)
{
	char *url, *e_owner, *e_repo;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, milestone);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_milestone(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_milestone_get_issues(struct gcli_ctx *ctx, char const *const owner,
                            char const *const repo, gcli_id const milestone,
                            struct gcli_issue_list *const out)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%"PRIid"/issues",
	                  gcli_get_apibase(ctx), e_owner, e_repo, milestone);

	free(e_repo);
	free(e_owner);
	/* URL is freed by the fetch_issues call */

	return gitlab_fetch_issues(ctx, url, -1, out);;
}

int
gitlab_create_milestone(struct gcli_ctx *ctx,
                        struct gcli_milestone_create_args const *args)
{
	char *url, *e_owner, *e_repo, *e_title, *json_body, *description = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(args->owner);
	e_repo = gcli_urlencode(args->repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	/* Escape and prepare the description if needed */
	if (args->description) {
		char *e_description = gcli_json_escape_cstr(args->description);
		description = sn_asprintf(", \"description\": \"%s\"", e_description);
		free(e_description);
	}

	e_title = gcli_json_escape_cstr(args->title);

	json_body = sn_asprintf("{"
	                        "    \"title\": \"%s\""
	                        "    %s"
	                        "}",
	                        e_title, description ? description : "");

	rc = gcli_fetch_with_method(ctx, "POST", url, json_body, NULL, NULL);

	free(json_body);
	free(description);
	free(url);
	free(e_title);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_delete_milestone(struct gcli_ctx *ctx, char const *const owner,
                        char const *const repo, gcli_id const milestone)
{
	char *url, *e_owner, *e_repo;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, milestone);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_milestone_set_duedate(struct gcli_ctx *ctx, char const *const owner,
                             char const *const repo, gcli_id const milestone,
                             char const *const date)
{
	char *url, *e_owner, *e_repo, norm_date[9] = {0};
	int rc = 0;

	rc = gcli_normalize_date(ctx, DATEFMT_GITLAB, date, norm_date,
	                         sizeof norm_date);
	if (rc < 0)
		return rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%"PRIid"?due_date=%s",
	                  gcli_get_apibase(ctx), e_owner, e_repo, milestone,
	                  norm_date);

	rc = gcli_fetch_with_method(ctx, "PUT", url, "", NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}
