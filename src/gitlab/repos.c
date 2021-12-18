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

#include <ghcli/gitlab/config.h>
#include <ghcli/gitlab/repos.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

static void
gitlab_parse_repo(json_stream *input, ghcli_repo *out)
{
    enum json_type  next       = JSON_NULL;
    enum json_type  key_type   = JSON_NULL;
    enum json_type  value_type = JSON_NULL;
    const char     *key        = NULL;

    if ((next = json_next(input)) != JSON_OBJECT)
        errx(1, "Expected an object for a repo");

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("path_with_namespace", key, len) == 0) {
            out->full_name = get_sv(input);
        } else if (strncmp("name", key, len) == 0) {
            out->name = get_sv(input);
        } else if (strncmp("owner", key, len) == 0) {
            out->owner = get_user_sv(input);
        } else if (strncmp("created_at", key, len) == 0) {
            out->date = get_sv(input);
        } else if (strncmp("visibility", key, len) == 0) {
            out->visibility = get_sv(input);
        } else if (strncmp("fork", key, len) == 0) {
            out->is_fork = get_bool(input);
        } else if (strncmp("id", key, len) == 0) {
            out->id = get_int(input);
        } else {
            value_type = json_next(input);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(input, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(input, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Repo object not closed");
}

void
gitlab_get_repo(
    sn_sv       owner,
    sn_sv       repo,
    ghcli_repo *out)
{
    /* GET /projects/:id */
    char               *url    = NULL;
    ghcli_fetch_buffer  buffer = {0};
    struct json_stream  stream = {0};

    url = sn_asprintf(
        "%s/projects/"SV_FMT"%%2F"SV_FMT,
        gitlab_get_apibase(),
        SV_ARGS(owner), SV_ARGS(repo));

    ghcli_fetch(url, NULL, &buffer);
    json_open_buffer(&stream, buffer.data, buffer.length);

    gitlab_parse_repo(&stream, out);

    json_close(&stream);
    free(buffer.data);
    free(url);
}

int
gitlab_get_repos(
    const char  *owner,
    int          max,
    ghcli_repo **out)
{
    char               *url      = NULL;
    char               *next_url = NULL;
    ghcli_fetch_buffer  buffer   = {0};
    struct json_stream  stream   = {0};
    enum  json_type     next     = JSON_NULL;
    int                 size     = 0;

    url = sn_asprintf("%s/users/%s/projects", gitlab_get_apibase(), owner);

    do {
        ghcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);
        json_set_streaming(&stream, 1);

        // TODO: Poor error message
        if ((next = json_next(&stream)) != JSON_ARRAY)
            errx(1,
                 "Expected array in response from API "
                 "but got something else instead");

        while ((next = json_peek(&stream)) != JSON_ARRAY_END) {
            *out = realloc(*out, sizeof(**out) * (size + 1));
            ghcli_repo *it = &(*out)[size++];
            gitlab_parse_repo(&stream, it);

            if (size == max)
                break;
        }

        free(url);
        free(buffer.data);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || size < max));

    free(url);

    return size;
}

int
gitlab_get_own_repos(
    int          max,
    ghcli_repo **out)
{
    char  *_account = NULL;
    sn_sv  account  = {0};
    int    n;

    account = gitlab_get_account();
    if (!account.length)
        errx(1, "error: gitlab.account is not set");

    _account = sn_sv_to_cstr(account);

    n = gitlab_get_repos(_account, max, out);

    free(_account);

    return n;
}

void
gitlab_repo_delete(
    const char *owner,
    const char *repo)
{
    char               *url    = NULL;
    ghcli_fetch_buffer  buffer = {0};

    url = sn_asprintf("%s/projects/%s%%2F%s",
                      gitlab_get_apibase(),
                      owner, repo);

    ghcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

    free(buffer.data);
    free(url);
}