/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/config.h>
#include <ghcli/repos.h>
#include <ghcli/github/repos.h>

#include <stdlib.h>

int
ghcli_get_repos(const char *owner, int max, ghcli_repo **out)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        return github_get_repos(owner, max, out);
    default:
        sn_unimplemented;
    }

    return -1;
}


static void
ghcli_print_repo(FILE *stream, ghcli_repo *repo)
{
    fprintf(
        stream,
        "%-4.4s  %-10.10s  %-16.16s  %-s\n",
        sn_bool_yesno(repo->is_fork),
        repo->visibility.data,
        repo->date.data,
        repo->full_name.data);
}

void
ghcli_print_repos_table(
    FILE                    *stream,
    enum ghcli_output_order  order,
    ghcli_repo              *repos,
    size_t                   repos_size)
{
    if (repos_size == 0) {
        fprintf(stream, "No repos\n");
        return;
    }

    fprintf(
        stream,
        "%-4.4s  %-10.10s  %-16.16s  %-s\n",
        "FORK", "VISBLTY", "DATE", "FULLNAME");

    if (order == OUTPUT_ORDER_SORTED) {
        for (size_t i = repos_size; i > 0; --i)
            ghcli_print_repo(stream, &repos[i - 1]);
    } else {
        for (size_t i = 0; i < repos_size; ++i)
            ghcli_print_repo(stream, &repos[i]);
    }
}

void
ghcli_repos_free(ghcli_repo *repos, size_t repos_size)
{
    for (size_t i = 0; i < repos_size; ++i) {
        free(repos[i].full_name.data);
        free(repos[i].name.data);
        free(repos[i].owner.data);
        free(repos[i].date.data);
        free(repos[i].visibility.data);
    }

    free(repos);
}

int
ghcli_get_own_repos(int max, ghcli_repo **out)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        return github_get_own_repos(max, out);
    default:
        sn_unimplemented;
    }

    return -1;
}

void
ghcli_repo_delete(const char *owner, const char *repo)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        github_repo_delete(owner, repo);
        break;
    default:
        sn_unimplemented;
    }
}
