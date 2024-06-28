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

#include <gcli/github/comments.h>
#include <gcli/github/config.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/comments.h>

int
github_perform_submit_comment(struct gcli_ctx *ctx,
                              struct gcli_submit_comment_opts opts)
{
	int rc = 0;
	struct gcli_jsongen gen = {0};
	char *e_owner = gcli_urlencode(opts.owner);
	char *e_repo = gcli_urlencode(opts.repo);
	char *payload = NULL, *url = NULL;

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "body");
		gcli_jsongen_string(&gen, opts.message);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"/comments",
	                  gcli_get_apibase(ctx), e_owner, e_repo, opts.target_id);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	free(payload);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
github_fetch_comments(struct gcli_ctx *ctx, char *url,
                      struct gcli_comment_list *const out)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->comments,
		.sizep = &out->comments_size,
		.parse = (parsefn)parse_github_comments,
		.max = -1,
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_get_comments(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue, struct gcli_comment_list *const out)
{
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *url = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%"PRIid"/comments",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);
	free(e_owner);
	free(e_repo);

	return github_fetch_comments(ctx, url, out);
}
