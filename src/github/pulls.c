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
#include <ghcli/curl.h>
#include <ghcli/github/pulls.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
parse_pull_entry(json_stream *input, ghcli_pull *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Issue Object");

    enum json_type key_type;
    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(input, &len);
        enum json_type  value_type = 0;

        if (strncmp("title", key, len) == 0)
            it->title = get_string(input);
        else if (strncmp("state", key, len) == 0)
            it->state = get_string(input);
        else if (strncmp("number", key, len) == 0)
            it->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            it->id = get_int(input);
        else if (strncmp("merged_at", key, len) == 0)
            it->merged = json_next(input) == JSON_STRING;
        else if (strncmp("user", key, len) == 0)
            it->creator = get_user(input);
        else {
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
}

int
github_get_prs(
    const char  *owner,
    const char  *reponame,
    bool         all,
    int          max,
    ghcli_pull **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *next_url    = NULL;

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls?state=%s",
        ghcli_config_get_apibase(),
        owner, reponame, all ? "all" : "open");

    do {
        ghcli_fetch(url, &next_url, &json_buffer);

        json_open_buffer(&stream, json_buffer.data, json_buffer.length);
        json_set_streaming(&stream, true);

        enum json_type next_token = json_next(&stream);

        while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {

            switch (next_token) {
            case JSON_ERROR:
                errx(1, "Parser error: %s", json_get_error(&stream));
                break;
            case JSON_OBJECT: {
                *out = realloc(*out, sizeof(ghcli_pull) * (count + 1));
                ghcli_pull *it = &(*out)[count];
                memset(it, 0, sizeof(ghcli_pull));
                parse_pull_entry(&stream, it);
                count += 1;
            } break;
            default:
                errx(1, "Unexpected json type in response");
                break;
            }

            if (count == max)
                break;
        }

        free(json_buffer.data);
        free(url);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || count < max));

    free(url);

    return count;
}

void
github_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number)
{
    char *url = NULL;
    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        ghcli_config_get_apibase(),
        owner, reponame, pr_number);
    ghcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");
    free(url);
}

void
github_pr_merge(
    FILE       *out,
    const char *owner,
    const char *reponame,
    int         pr_number)
{
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    const char         *data        = "{}";
    enum json_type      next;
    size_t              len;
    const char         *message;
    const char         *key;

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/merge",
        ghcli_config_get_apibase(),
        owner, reponame, pr_number);
    ghcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    next = json_next(&stream);

    while ((next = json_next(&stream)) == JSON_STRING) {
        key = json_get_string(&stream, &len);

        if (strncmp(key, "message", len) == 0) {

            next = json_next(&stream);
            message  = json_get_string(&stream, &len);

            fprintf(out, "%.*s\n", (int)len, message);

            json_close(&stream);
            free(json_buffer.data);
            free(url);

            return;
        } else {
            next = json_next(&stream);
        }
    }
}

void
github_pr_close(const char *owner, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        ghcli_config_get_apibase(),
        owner, reponame, pr_number);
    data = sn_asprintf("{ \"state\": \"closed\"}");

    ghcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}

void
github_pr_reopen(const char *owner, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        ghcli_config_get_apibase(),
        owner, reponame, pr_number);
    data = sn_asprintf("{ \"state\": \"open\"}");

    ghcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}
