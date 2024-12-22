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

#include <gcli/forges.h>
#include <gcli/github/repos.h>
#include <gcli/repos.h>
#include <gcli/cmd/table.h>

#include <stdlib.h>

int
gcli_get_repos(struct gcli_ctx *ctx, char const *owner, int const max,
               struct gcli_repo_list *const out)
{
	gcli_null_check_call(get_repos, ctx, owner, max, out);
}

void
gcli_repo_free(struct gcli_repo *it)
{
	free(it->full_name);
	free(it->name);
	free(it->owner);
	free(it->visibility);
	memset(it, 0, sizeof(*it));
}

void
gcli_repos_free(struct gcli_repo_list *const list)
{
	for (size_t i = 0; i < list->repos_size; ++i) {
		gcli_repo_free(&list->repos[i]);
	}

	free(list->repos);

	list->repos = NULL;
	list->repos_size = 0;
}

int
gcli_repo_delete(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(repo_delete, ctx, path);
}

int
gcli_repo_create(struct gcli_ctx *ctx, struct gcli_repo_create_options const *options,
                 struct gcli_repo *out)
{
	gcli_null_check_call(repo_create, ctx, options, out);
}

int
gcli_repo_set_visibility(struct gcli_ctx *ctx, struct gcli_path const *const path,
                         gcli_repo_visibility vis)
{
	gcli_null_check_call(repo_set_visibility, ctx, path, vis);
}
