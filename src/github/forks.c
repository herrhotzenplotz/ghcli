/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/github/forks.h>
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/forks.h>

int
github_get_forks(struct  gcli_ctx *ctx, struct gcli_path const *const path,
                 int const max, struct gcli_fork_list *const list)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &list->forks,
		.sizep = &list->forks_size,
		.max = max,
		.parse = (parsefn)(parse_github_forks),
	};

	rc = github_repo_make_url(ctx, path, &url, "/forks");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_fork_create(struct gcli_ctx *ctx,
                   struct gcli_path const *const repo_path,
                   char const *const in)
{
	char *url = NULL;
	char *post_data = NULL;
	int rc = 0;
	bool is_org = false;

	if (in) {
		char *const e_in = gcli_urlencode(in);
		rc = github_user_is_org(ctx, e_in);
		free(e_in);

		if (rc < 0)
			return rc;

		is_org = rc;
	}

	rc = github_repo_make_url(ctx, repo_path, &url, "/forks");
	if (rc < 0)
		return rc;

	if (is_org) {
		struct gcli_jsongen gen = {0};

		gcli_jsongen_init(&gen);
		gcli_jsongen_begin_object(&gen);
		{
			gcli_jsongen_objmember(&gen, "organization");
			gcli_jsongen_string(&gen, in);
		}
		gcli_jsongen_end_object(&gen);

		post_data = gcli_jsongen_to_string(&gen);
		gcli_jsongen_free(&gen);
	}

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, NULL);

	free(url);
	free(post_data);

	return rc;
}
