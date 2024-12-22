/*
 * Copyright 2023-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/github/milestones.h>

#include <gcli/curl.h>
#include <gcli/date_time.h>
#include <gcli/github/issues.h>
#include <gcli/github/repos.h>
#include <gcli/json_util.h>

#include <templates/github/milestones.h>

#include <pdjson/pdjson.h>

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* Given a repository path make the url to milestones */
static int
github_milestones_make_url(struct gcli_ctx *const ctx,
                           struct gcli_path const *const path,
                           char const *const suffix, char **url)
{
	int rc = 0;

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s/milestones%s",
		                   gcli_get_apibase(ctx),
		                   e_owner, e_repo, suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s/milestones%s", path->data.as_url,
		                   suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for milestones");
	} break;
	}

	return rc;
}

int
github_get_milestones(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      int const max, struct gcli_milestone_list *const out)
{
	char *url;
	int rc;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->milestones,
		.sizep = &out->milestones_size,
		.parse = (parsefn)parse_github_milestones,
		.max = max,
	};

	rc = github_milestones_make_url(ctx, path, "", &url);
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_milestone_make_url(struct gcli_ctx *ctx,
                          struct gcli_path const *const path,
                          char **url,
                          char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = sn_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s/milestone/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   path->data.as_default.id, suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for milestones");
	} break;
	}

	free(suffix);

	return rc;
}

int
github_get_milestone(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     struct gcli_milestone *const out)
{
	char *url;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	rc = github_milestone_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_milestone(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_milestone_get_issues(struct gcli_ctx *ctx,
                            struct gcli_path const *const path,
                            struct gcli_issue_list *const out)
{
	char *url = NULL;
	int rc = 0;

	if (path->kind != GCLI_PATH_DEFAULT)
		return gcli_error(ctx, "unsupported path type getting milestone's issues");

	rc = github_repo_make_url(ctx, path, &url,
	                          "/issues?milestone=%"PRIid"&state=all",
	                          path->data.as_default.id);
	if (rc < 0)
		return rc;

	return github_fetch_issues(ctx, url, -1, out);
}

int
github_create_milestone(struct gcli_ctx *ctx,
                        struct gcli_milestone_create_args const *args)
{
	char *url, *e_owner, *e_repo;
	char *json_body, *description;
	int rc = 0;

	e_owner = gcli_urlencode(args->owner);
	e_repo = gcli_urlencode(args->repo);

	if (args->description) {
		/* This is fine :-) */
		char *e_description = gcli_json_escape_cstr(args->description);
		description = sn_asprintf(",\"description\": \"%s\"", e_description);
		free(e_description);
	} else {
		description = strdup("");
	}

	json_body = sn_asprintf(
		"{"
		"    \"title\"      : \"%s\""
		"    %s"
		"}", args->title, description);

	url = sn_asprintf("%s/repos/%s/%s/milestones",
	                  gcli_get_apibase(ctx), e_owner, e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, json_body, NULL, NULL);

	free(json_body);
	free(description);
	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
github_delete_milestone(struct gcli_ctx *ctx,
                        struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = github_milestone_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
github_milestone_set_duedate(struct gcli_ctx *ctx,
                             struct gcli_path const *const path,
                             char const *const date)
{
	char *url = NULL, *payload = NULL, norm_date[21] = {0};
	int rc = 0;

	rc = gcli_normalize_date(ctx, DATEFMT_ISO8601, date, norm_date,
	                         sizeof norm_date);
	if (rc < 0)
		return rc;

	rc = github_milestone_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	payload = sn_asprintf("{ \"due_on\": \"%s\"}", norm_date);
	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);

	return rc;
}
