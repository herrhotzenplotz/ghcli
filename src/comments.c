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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/editor.h>

#include <gcli/comments.h>
#include <gcli/forges.h>
#include <gcli/github/comments.h>
#include <gcli/json_util.h>
#include <sn/sn.h>

void
gcli_comment_free(struct gcli_comment *const it)
{
	free(it->author);
	free(it->date);
	free(it->body);
}

void
gcli_comments_free(struct gcli_comment_list *const list)
{
	for (size_t i = 0; i < list->comments_size; ++i)
		gcli_comment_free(&list->comments[i]);

	free(list->comments);
	list->comments = NULL;
	list->comments_size = 0;
}

int
gcli_get_issue_comments(struct gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const issue, struct gcli_comment_list *out)
{
	gcli_null_check_call(get_issue_comments, ctx, owner, repo, issue, out);
}

int
gcli_get_pull_comments(struct gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id const pull, struct gcli_comment_list *out)
{
	gcli_null_check_call(get_pull_comments, ctx, owner, repo, pull, out);
}

int
gcli_comment_submit(struct gcli_ctx *ctx, struct gcli_submit_comment_opts opts)
{
	gcli_null_check_call(perform_submit_comment, ctx, opts);
}
