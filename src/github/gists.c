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
#include <gcli/cmd/table.h>
#include <gcli/curl.h>
#include <gcli/github/gists.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <gcli/github/config.h>

#include <pdjson/pdjson.h>

#include <templates/github/gists.h>

/* /!\ Before changing this, see comment in gists.h /!\ */
int
parse_github_gist_files_idiot_hack(struct gcli_ctx *ctx, json_stream *stream,
                                   struct gcli_gist *const gist)
{
	(void) ctx;

	enum json_type next = JSON_NULL;

	gist->files = NULL;
	gist->files_size = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected Gist Files Object");

	while ((next = json_next(stream)) == JSON_STRING) {
		gist->files = realloc(gist->files, sizeof(*gist->files) * (gist->files_size + 1));
		struct gcli_gist_file *it = &gist->files[gist->files_size++];
		if (parse_github_gist_file(ctx, stream, it) < 0)
			return -1;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed Gist Files Object");

	return 0;
}

int
gcli_get_gists(struct gcli_ctx *ctx, char const *user, int const max,
               struct gcli_gist_list *const list)
{
	char *url = NULL;
	struct gcli_fetch_list_ctx fl = {
		.listp = &list->gists,
		.sizep = &list->gists_size,
		.parse = (parsefn)(parse_github_gists),
		.max = max,
	};

	if (user)
		url = sn_asprintf("%s/users/%s/gists", gcli_get_apibase(ctx), user);
	else
		url = sn_asprintf("%s/gists", gcli_get_apibase(ctx));

	return gcli_fetch_list(ctx, url, &fl);
}

int
gcli_get_gist(struct gcli_ctx *ctx, char const *gist_id, struct gcli_gist *out)
{
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	url = sn_asprintf("%s/gists/%s", gcli_get_apibase(ctx), gist_id);
	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		parse_github_gist(ctx, &stream, out);

		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

#define READ_SZ 4096
static char *
read_file(FILE *f)
{
	size_t size = 0;
	char *out = NULL;

	while (!feof(f) && !ferror(f)) {
		out = realloc(out, size + READ_SZ);
		size_t bytes_read = fread(out + size, 1, READ_SZ, f);
		if (bytes_read == 0)
			break;

		size += bytes_read;
	}

	if (out) {
		out = realloc(out, size + 1);
		out[size] = '\0';
	}

	if (ferror(f)) {
		free(out);
		out = NULL;
	}

	return out;
}

int
gcli_create_gist(struct gcli_ctx *ctx, struct gcli_new_gist opts)
{
	char *content = NULL;
	char *post_data = NULL;
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};

	/* Read in the file content. this may come from stdin this we don't know
	 * the size in advance. */
	content = read_file(opts.file);
	if (content == NULL)
		return gcli_error(ctx, "failed to read from input file");

	/* This API is documented very badly. In fact, I dug up how you're
	 * supposed to do this from
	 * https://github.com/phadej/github/blob/master/src/GitHub/Data/Gists.hs
	 *
	 * From this we can infer that we're supposed to create a JSON
	 * object like so:
	 *
	 * {
	 *  "description": "foobar",
	 *  "public": true,
	 *  "files": {
	 *   "barf.exe": {
	 *       "content": "#!/bin/sh\necho This file cannot be run in DOS mode"
	 *   }
	 *  }
	 * }
	 */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "description");
		gcli_jsongen_string(&gen, opts.gist_description);

		gcli_jsongen_objmember(&gen, "public");
		gcli_jsongen_bool(&gen, true);

		gcli_jsongen_objmember(&gen, "files");
		gcli_jsongen_begin_object(&gen);
		{
			gcli_jsongen_objmember(&gen, opts.file_name);
			gcli_jsongen_begin_object(&gen);
			{
				gcli_jsongen_objmember(&gen, "content");
				gcli_jsongen_string(&gen, content);
			}
			gcli_jsongen_end_object(&gen);
		}
		gcli_jsongen_end_object(&gen);
	}
	gcli_jsongen_end_object(&gen);

	post_data = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* Generate URL */
	url = sn_asprintf("%s/gists", gcli_get_apibase(ctx));

	/* Perferm fetch */
	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, &buffer);

	gcli_fetch_buffer_free(&buffer);
	free(content);
	free(url);
	free(post_data);

	return rc;
}

int
gcli_delete_gist(struct gcli_ctx *ctx, char const *gist_id)
{
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	url = sn_asprintf("%s/gists/%s", gcli_get_apibase(ctx), gist_id);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, &buffer);

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

void
gcli_gist_free(struct gcli_gist *g)
{
	free(g->id);
	free(g->owner);
	free(g->url);
	free(g->date);
	free(g->git_pull_url);
	free(g->description);

	for (size_t j = 0; j < g->files_size; ++j) {
		free(g->files[j].filename);
		free(g->files[j].language);
		free(g->files[j].url);
		free(g->files[j].type);
	}

	free(g->files);

	memset(g, 0, sizeof(*g));
}

void
gcli_gists_free(struct gcli_gist_list *const list)
{
	for (size_t i = 0; i < list->gists_size; ++i)
		gcli_gist_free(&list->gists[i]);

	free(list->gists);

	list->gists = NULL;
	list->gists_size = 0;
}
