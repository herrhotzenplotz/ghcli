/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <config.h>

#include <gcli/bugzilla/bugs.h>

#include <sn/sn.h>

#include <templates/bugzilla/bugs.h>

#include <gcli/base64.h>
#include <gcli/curl.h>
#include <gcli/json_gen.h>

#include <assert.h>

int
bugzilla_get_bugs(struct gcli_ctx *ctx, char const *product, char const *component,
                  struct gcli_issue_fetch_details const *details, int const max,
                  struct gcli_issue_list *out)
{
	char *url, *e_product = NULL, *e_component = NULL, *e_author = NULL,
	     *e_query = NULL;
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	if (product) {
		char *tmp = gcli_urlencode(product);
		e_product = sn_asprintf("&product=%s", tmp);
		free(tmp);
	}

	if (component) {
		char *tmp = gcli_urlencode(component);
		e_component = sn_asprintf("&component=%s", tmp);
		free(tmp);
	}

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("&creator=%s", tmp);
		free(tmp);
	}

	if (details->search_term) {
		char *tmp = gcli_urlencode(details->search_term);
		e_query = sn_asprintf("&quicksearch=%s", tmp);
		free(tmp);
	}

	/* TODO: handle the max = -1 case */
	/* Note(Nico): Most of the options here are not very well
	 * documented. Specifically the order= parameter I have figured out by
	 * reading the code and trying things until it worked. */
	url = sn_asprintf("%s/rest/bug?order=bug_id%%20DESC%%2C&limit=%d%s%s%s%s%s",
	                  gcli_get_apibase(ctx), max,
	                  details->all ? "&status=All" : "&status=Open&status=New",
	                  e_product ? e_product : "",
	                  e_component ? e_component : "",
	                  e_author ? e_author : "",
	                  e_query ? e_query : "");

	free(e_query);
	free(e_product);
	free(e_component);
	free(e_author);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_bugzilla_bugs(ctx, &stream, out);

		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

int
bugzilla_bug_get_comments(struct gcli_ctx *const ctx, char const *const product,
                          char const *const component, gcli_id const bug_id,
                          struct gcli_comment_list *out)
{
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	char *url = NULL;

	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/comment?include_fields=_all",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_comments(ctx, &stream, out);
	json_close(&stream);

	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

static int
bugzilla_bug_get_op(struct gcli_ctx *ctx, gcli_id const bug_id, char **out)
{
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	char *url = NULL;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/comment?include_fields=_all",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bug_op(ctx, &stream, out);
	json_close(&stream);

	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

int
bugzilla_get_bug(struct gcli_ctx *ctx, char const *product,
                 char const *component, gcli_id bug_id, struct gcli_issue *out)
{
	int rc = 0;
	char *url;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_issue_list list = {0};
	struct json_stream stream = {0};

	/* XXX should we warn if product or component is set? */
	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug?limit=1&id=%"PRIid, gcli_get_apibase(ctx), bug_id);
	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bugs(ctx, &stream, &list);

	if (rc < 0)
		goto error_parse;

	if (list.issues_size == 0) {
		rc = gcli_error(ctx, "no bug with id %"PRIid, bug_id);
		goto error_no_such_bug;
	}

	if (list.issues_size > 0) {
		assert(list.issues_size == 1);
		memcpy(out, &list.issues[0], sizeof(*out));
	}

	/* don't use gcli_issues_free because it frees data behind pointers we
	 * just copied */
	free(list.issues);

	/* The OP is in the comments. Fetch it separately. */
	rc = bugzilla_bug_get_op(ctx, bug_id, &out->body);

error_no_such_bug:
error_parse:
	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

int
bugzilla_bug_get_attachments(struct gcli_ctx *ctx, char const *const product,
                             char const *const component, gcli_id const bug_id,
                             struct gcli_attachment_list *const out)
{
	int rc = 0;
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/attachment",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bug_attachments(ctx, &stream, out);
	json_close(&stream);

	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}

static void
add_extra_options(struct gcli_nvlist const *list, struct gcli_jsongen *gen)
{
	static struct extra_opt {
		char const *json_name;
		char const *cli_name;
		char const *default_value;
	} extra_opts[] = {
		{ .json_name = "op_sys",
		  .cli_name = "os",
		  .default_value = "All" },
		{ .json_name = "rep_platform",
		  .cli_name = "hardware",
		  .default_value = "All" },
		{ .json_name = "version",
		  .cli_name = "version",
		  .default_value = "unspecified" },
	};
	static size_t extra_opts_size = ARRAY_SIZE(extra_opts);

	for (size_t i = 0; i < extra_opts_size; ++i) {
		struct extra_opt const *o = &extra_opts[i];
		char const *const val = gcli_nvlist_find_or(
			list, o->json_name, o->default_value);

		gcli_jsongen_objmember(gen, o->json_name);
		gcli_jsongen_string(gen, val);
	}
}

int
bugzilla_bug_submit(struct gcli_ctx *const ctx,
                    struct gcli_submit_issue_options *const opts,
                    struct gcli_issue *const out)
{
	char *payload = NULL, *url = NULL;
	char *token; /* bugzilla wants the api token as a parameter in the url or the json payload */
	char const *product = opts->owner, *component = opts->repo,
	           *summary = opts->title, *description = opts->body;
	struct gcli_jsongen gen = {0};
	struct gcli_fetch_buffer buffer = {0}, *_buffer = NULL;
	int rc = 0;

	/* prepare data for payload generation */
	if (product == NULL)
		return gcli_error(ctx, "product must not be empty");

	if (component == NULL)
		return gcli_error(ctx, "component must not be empty");

	token = gcli_get_token(ctx);
	if (!token)
		return gcli_error(ctx, "creating bugs on bugzilla requires a token");

	/* generate payload */
	rc = gcli_jsongen_init(&gen);
	if (rc < 0) {
		gcli_error(ctx, "failed to init json generator");
		goto err_jsongen_init;
	}

	/*
	 * {
	 *    "product" : "TestProduct",
	 *    "component" : "TestComponent",
	 *    "summary" : "'This is a test bug - please disregard",
	 *    "description": ...,
	 * } */
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "product");
		gcli_jsongen_string(&gen, product);

		gcli_jsongen_objmember(&gen, "component");
		gcli_jsongen_string(&gen, component);

		gcli_jsongen_objmember(&gen, "summary");
		gcli_jsongen_string(&gen, summary);

		if (description) {
			gcli_jsongen_objmember(&gen, "description");
			gcli_jsongen_string(&gen, description);
		}

		gcli_jsongen_objmember(&gen, "api_key");
		gcli_jsongen_string(&gen, token);

		add_extra_options(&opts->extra, &gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* generate url and perform request */
	url = sn_asprintf("%s/rest/bug", gcli_get_apibase(ctx));

	if (out)
		_buffer = &buffer;

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, _buffer);
	if (out && rc == 0) {
		struct json_stream stream = {0};
		gcli_id id = 0;

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_bugzilla_bug_creation_result(ctx, &stream, &id);
		json_close(&stream);

		if (rc == 0)
			rc = bugzilla_get_bug(ctx, NULL, NULL, id, out);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);
	free(payload);

err_jsongen_init:
	free(token);

	return rc;
}
