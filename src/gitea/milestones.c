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

#include <gcli/gitea/milestones.h>
#include <gcli/gitea/repos.h>

#include <gcli/curl.h>

#include <gcli/github/issues.h>
#include <gcli/github/milestones.h>

#include <templates/gitea/milestones.h>

#include <pdjson/pdjson.h>

#include <stdarg.h>

int
gitea_milestone_make_url(struct gcli_ctx *ctx,
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

		*url = sn_asprintf("%s/repos/%s/%s/milestones/%"PRIid"%s",
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
gitea_get_milestones(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     int const max, struct gcli_milestone_list *const out)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->milestones,
		.sizep = &out->milestones_size,
		.max = max,
		.parse = (parsefn)(parse_gitea_milestones),
	};

	rc = gitea_repo_make_url(ctx, path, &url, "/milestones");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitea_get_milestone(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_milestone *const out)
{
	char *url;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};

	rc = gitea_milestone_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitea_milestone(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

int
gitea_create_milestone(struct gcli_ctx *ctx,
                       struct gcli_milestone_create_args const *args)
{
	return github_create_milestone(ctx, args);
}

int
gitea_milestone_get_issues(struct gcli_ctx *ctx,
                           struct gcli_path const *const path,
                           struct gcli_issue_list *const out)
{
	char *url = NULL;
	int rc = 0;

	if (path->kind != GCLI_PATH_DEFAULT)
		return gcli_error(ctx, "unsupported path kind for getting issues of Gitea milestone");

	rc = gitea_repo_make_url(ctx, path, &url,
	                         "/issues?state=all&milestones=%"PRIid,
	                         path->data.as_default.id);
	if (rc < 0)
		return rc;

	return github_fetch_issues(ctx, url, -1, out);
}

int
gitea_delete_milestone(struct gcli_ctx *ctx,
                       struct gcli_path const *const path)
{
	return github_delete_milestone(ctx, path);
}

int
gitea_milestone_set_duedate(struct gcli_ctx *ctx,
                            struct gcli_path const *const path,
                            char const *const date)
{
	return github_milestone_set_duedate(ctx, path, date);
}
