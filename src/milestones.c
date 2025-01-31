/*
 * Copyright 2023-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/milestones.h>

int
gcli_get_milestones(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    int const max, struct gcli_milestone_list *const out)
{
	gcli_null_check_call(get_milestones, ctx, path, max, out);
}

int
gcli_get_milestone(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   struct gcli_milestone *const out)
{
	gcli_null_check_call(get_milestone, ctx, path, out);
}

int
gcli_create_milestone(struct gcli_ctx *ctx,
                      struct gcli_milestone_create_args const *args)
{
	gcli_null_check_call(create_milestone, ctx, args);
}

int
gcli_delete_milestone(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(delete_milestone, ctx, path);
}

void
gcli_free_milestone(struct gcli_milestone *const it)
{
	free(it->title);
	it->title = NULL;
	free(it->state);
	it->state = NULL;

	free(it->description);
	it->description = NULL;

	free(it->web_url);
	it->web_url = NULL;
}

void
gcli_free_milestones(struct gcli_milestone_list *const it)
{
	for (size_t i = 0; i < it->milestones_size; ++i)
		gcli_free_milestone(&it->milestones[i]);

	free(it->milestones);
	it->milestones = NULL;
	it->milestones_size = 0;
}

int
gcli_milestone_get_issues(struct  gcli_ctx *ctx,
                          struct gcli_path const *const milestone_path,
                          struct gcli_issue_list *const out)
{
	gcli_null_check_call(get_milestone_issues, ctx, milestone_path, out);
}

int
gcli_milestone_set_duedate(struct  gcli_ctx *ctx,
                           struct gcli_path const *const path,
                           char const *const date)
{
	gcli_null_check_call(milestone_set_duedate, ctx, path, date);
}
