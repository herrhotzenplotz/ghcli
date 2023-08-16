/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/colour.h>
#include <gcli/forges.h>
#include <gcli/forks.h>
#include <gcli/github/forks.h>
#include <gcli/cmd/table.h>

int
gcli_get_forks(gcli_ctx *ctx, char const *owner, char const *repo,
               int const max, gcli_fork_list *const out)
{
	return gcli_forge(ctx)->get_forks(ctx, owner, repo, max, out);
}

void
gcli_print_forks(gcli_ctx *ctx, enum gcli_output_flags const flags,
                 gcli_fork_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "OWNER",    .type = GCLI_TBLCOLTYPE_SV,  .flags = GCLI_TBLCOL_BOLD },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
		{ .name = "FORKS",    .type = GCLI_TBLCOLTYPE_INT, .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
	};

	if (list->forks_size == 0) {
		puts("No forks");
		return;
	}

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->forks_size)
		n = list->forks_size;
	else
		n = max;

	table = gcli_tbl_begin(ctx, cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->forks[n-i-1].owner,
			                 list->forks[n-i-1].date,
			                 list->forks[n-i-1].forks,
			                 list->forks[n-i-1].full_name);
		}
	} else {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->forks[i].owner,
			                 list->forks[i].date,
			                 list->forks[i].forks,
			                 list->forks[i].full_name);
		}
	}

	gcli_tbl_end(table);
}

int
gcli_fork_create(gcli_ctx *ctx, char const *owner, char const *repo,
                 char const *_in)
{
	return gcli_forge(ctx)->fork_create(ctx, owner, repo, _in);
}

void
gcli_forks_free(gcli_fork_list *const list)
{
	for (size_t i = 0; i < list->forks_size; ++i) {
		free(list->forks[i].full_name.data);
		free(list->forks[i].owner.data);
		free(list->forks[i].date.data);
	}

	free(list->forks);

	list->forks = NULL;
	list->forks_size = 0;
}
