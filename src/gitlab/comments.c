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

#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/gitlab/comments.h>

int
gitlab_perform_submit_comment(struct gcli_ctx *ctx,
                              struct gcli_submit_comment_opts const *const opts)
{
	char *url = NULL, *payload = NULL, *e_owner = NULL, *e_repo = NULL;
	char const *type = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	e_owner = gcli_urlencode(opts->owner);
	e_repo = gcli_urlencode(opts->repo);

	switch (opts->target_type) {
	case ISSUE_COMMENT:
		type = "issues";
		break;
	case PR_COMMENT:
		type = "merge_requests";
		break;
	}

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "body");
		gcli_jsongen_string(&gen, opts->message);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	url = sn_asprintf("%s/projects/%s%%2F%s/%s/%"PRIid"/notes",
	                  gcli_get_apibase(ctx), e_owner, e_repo, type,
	                  opts->target_id);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(payload);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

static void
reverse_comment_list(struct gcli_comment_list *const list)
{
	struct gcli_comment *reversed =
		calloc(list->comments_size, sizeof *list->comments);

	for (size_t i = 0; i < list->comments_size; ++i)
		reversed[i] = list->comments[list->comments_size - i - 1];

	free(list->comments);
	list->comments = reversed;
}

int
gitlab_fetch_comments(struct gcli_ctx *ctx, char *url,
                      struct gcli_comment_list *const out)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->comments,
		.sizep = &out->comments_size,
		.parse = (parsefn)parse_gitlab_comments,
		.max = -1,
	};

	/* Comments in the resulting list are in reverse on Gitlab
	 * (most recent is first). */
	int const rc = gcli_fetch_list(ctx, url, &fl);
	if (rc == 0)
		reverse_comment_list(out);

	return rc;
}

int
gitlab_get_mr_comments(struct gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id const mr, struct gcli_comment_list *const out)
{
	char *e_owner = gcli_urlencode(owner);
	char *e_repo = gcli_urlencode(repo);

	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%"PRIid"/notes",
		gcli_get_apibase(ctx),
		e_owner, e_repo, mr);

	free(e_owner);
	free(e_repo);

	return gitlab_fetch_comments(ctx, url, out);
}

int
gitlab_get_issue_comments(struct gcli_ctx *ctx, char const *owner,
                          char const *repo, gcli_id const issue,
                          struct gcli_comment_list *const out)
{
	char *e_owner = gcli_urlencode(owner);
	char *e_repo = gcli_urlencode(repo);

	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/issues/%"PRIid"/notes",
		gcli_get_apibase(ctx),
		e_owner, e_repo, issue);

	free(e_owner);
	free(e_repo);

	return gitlab_fetch_comments(ctx, url, out);
}
