/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/table.h>

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <assert.h>

#include <templates/gitlab/pipelines.h>

static int
fetch_pipelines(struct gcli_ctx *ctx, char *url, int const max,
                struct gitlab_pipeline_list *const list)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &list->pipelines,
		.sizep = &list->pipelines_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_pipelines),
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_pipelines(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     int const max, struct gitlab_pipeline_list *const list)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_repo_make_url(ctx, path, &url, "/pipelines");
	if (rc < 0)
		return rc;

	return fetch_pipelines(ctx, url, max, list);
}

static int
gitlab_pipeline_make_url(struct gcli_ctx *ctx,
                         struct gcli_path const *const path,
                         char **url,
                         char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = sn_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo = NULL;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/projects/%s%%2F%s/pipelines/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   path->data.as_default.id,
		                   suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab pipelines");
	} break;
	}

	free(suffix);

	return rc;
}

int
gitlab_get_pipeline(struct gcli_ctx *ctx,
                    struct gcli_path const *const pipeline_path,
                    struct gitlab_pipeline *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);

		rc = parse_gitlab_pipeline(ctx, &stream, out);

		json_close(&stream);
		gcli_fetch_buffer_free(&buffer);
	}

	free(url);

	return rc;
}

int
gitlab_get_mr_pipelines(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        struct gitlab_pipeline_list *const list)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_mr_make_url(ctx, path, &url, "/pipelines");
	if (rc < 0)
		return rc;

	/* fetch everything */
	return fetch_pipelines(ctx, url, -1, list);
}

void
gitlab_pipeline_free(struct gitlab_pipeline *pipeline)
{
	free(pipeline->status);
	free(pipeline->ref);
	free(pipeline->sha);
	free(pipeline->source);
}

void
gitlab_pipelines_free(struct gitlab_pipeline_list *const list)
{
	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gitlab_pipeline_free(&list->pipelines[i]);
	}
	free(list->pipelines);

	list->pipelines = NULL;
	list->pipelines_size = 0;
}

int
gitlab_get_pipeline_jobs(struct gcli_ctx *ctx,
                         struct gcli_path const *const pipeline_path,
                         int const max, struct gitlab_job_list *const out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->jobs,
		.sizep = &out->jobs_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_jobs),
	};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "/jobs");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_pipeline_children(struct gcli_ctx *ctx,
                             struct gcli_path const *const pipeline_path,
                             int count,
                             struct gitlab_pipeline_list *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->pipelines,
		.sizep = &out->pipelines_size,
		.max = count,
		.parse = (parsefn)(parse_gitlab_pipeline_children),
	};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "/bridges");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

void
gitlab_free_job(struct gitlab_job *const job)
{
	free(job->status);
	free(job->stage);
	free(job->name);
	free(job->ref);
	free(job->runner_name);
	free(job->runner_description);
}

void
gitlab_free_jobs(struct gitlab_job_list *list)
{
	for (size_t i = 0; i < list->jobs_size; ++i)
		gitlab_free_job(&list->jobs[i]);

	free(list->jobs);

	list->jobs = NULL;
	list->jobs_size = 0;
}

static int
gitlab_job_make_url(struct gcli_ctx *ctx,
                    struct gcli_path const *const path,
                    char **url,
                    char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = sn_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo = NULL;

		e_owner = gcli_urlencode(path->data.as_default.owner);
		e_repo = gcli_urlencode(path->data.as_default.repo);

		*url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   path->data.as_default.id,
		                   suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->data.as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab jobs");
	} break;
	}

	free(suffix);

	return rc;
}

int
gitlab_job_get_log(struct gcli_ctx *ctx, struct gcli_path const *const job_path,
                   FILE *stream)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, job_path, &url, "/trace");
	if (rc < 0)
		return rc;

	rc = gcli_curl(ctx, stream, url, NULL);

	free(url);

	return rc;
}

int
gitlab_get_job(struct gcli_ctx *ctx, struct gcli_path const *const job_path,
               struct gitlab_job *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, job_path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);
		parse_gitlab_job(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	free(url);

	return rc;
}

int
gitlab_job_cancel(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, path, &url, "/cancel");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_job_retry(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	int rc = 0;
	char *url = NULL;

	rc = gitlab_job_make_url(ctx, path, &url, "/retry");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_job_download_artifacts(struct gcli_ctx *ctx,
                              struct gcli_path const *const job_path,
                              char const *const outfile)
{
	FILE *f = NULL;
	char *url = NULL;
	int rc = 0;

	f = fopen(outfile, "wb");
	if (f == NULL)
		rc = gcli_error(ctx, "failed to open output file %s", outfile);

	if (rc == 0)
		rc = gitlab_job_make_url(ctx, job_path, &url, "/artifacts");

	if (rc == 0)
		rc = gcli_curl(ctx, f, url, "application/zip");

	if (f)
		fclose(f);

	free(url);

	return rc;
}
