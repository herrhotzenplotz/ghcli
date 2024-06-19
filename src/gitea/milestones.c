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

#include <gcli/gitea/milestones.h>

#include <gcli/curl.h>

#include <gcli/github/issues.h>
#include <gcli/github/milestones.h>

#include <templates/gitea/milestones.h>

#include <pdjson/pdjson.h>

int
gitea_get_milestones(struct gcli_ctx *ctx, char const *const owner,
                     char const *const repo, int const max,
                     struct gcli_milestone_list *const out)
{
	char *url;
	char *e_owner, *e_repo;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->milestones,
		.sizep = &out->milestones_size,
		.max = max,
		.parse = (parsefn)(parse_gitea_milestones),
	};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones",
	                  gcli_get_apibase(ctx), e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitea_get_milestone(struct gcli_ctx *ctx, char const *const owner,
                    char const *const repo, gcli_id const milestone,
                    struct gcli_milestone *const out)
{
	char *url, *e_owner, *e_repo;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, milestone);

	free(e_owner);
	free(e_repo);

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
gitea_milestone_get_issues(struct gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const milestone,
                           struct gcli_issue_list *const out)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues?state=all&milestones=%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, milestone);

	free(e_repo);
	free(e_owner);

	return github_fetch_issues(ctx, url, -1, out);
}

int
gitea_delete_milestone(struct gcli_ctx *ctx, char const *const owner,
                       char const *const repo, gcli_id const milestone)
{
	return github_delete_milestone(ctx, owner, repo, milestone);
}

int
gitea_milestone_set_duedate(struct gcli_ctx *ctx, char const *const owner,
                            char const *const repo, gcli_id const milestone,
                            char const *const date)
{
	return github_milestone_set_duedate(ctx, owner, repo, milestone, date);
}
