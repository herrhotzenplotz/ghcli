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

#include <config.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/table.h>

#include <gcli/forges.h>

#include <gcli/gitlab/pipelines.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pipelines [-o owner -r repo] [-n number]\n");
	fprintf(stderr, "       gcli pipelines [-o owner -r repo] -p pipeline pipeline-actions...\n");
	fprintf(stderr, "       gcli pipelines [-o owner -r repo] -j job [-n number] job-actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner                 The repository owner\n");
	fprintf(stderr, "  -r repo                  The repository name\n");
	fprintf(stderr, "  -p pipeline              Run actions for the given pipeline\n");
	fprintf(stderr, "  -j job                   Run actions for the given job\n");
	fprintf(stderr, "  -n number                Number of pipelines to fetch (-1 = everything)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "PIPELINE ACTIONS:\n");
	fprintf(stderr, "  jobs                     Print the list of jobs of this pipeline\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "JOB ACTIONS:\n");
	fprintf(stderr, "  status                   Display status information\n");
	fprintf(stderr, "  artifacts [-o filename]  Download a zip archive of the artifacts of the given job\n");
	fprintf(stderr, "                           (default output filename: artifacts.zip)\n");
	fprintf(stderr, "  log                      Display job log\n");
	fprintf(stderr, "  cancel                   Cancel the job\n");
	fprintf(stderr, "  retry                    Retry the given job\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

int
gitlab_mr_pipelines(char const *owner, char const *repo, int const mr_id)
{
	struct gitlab_pipeline_list list = {0};
	int rc = 0;

	rc = gitlab_get_mr_pipelines(g_clictx, owner, repo, mr_id, &list);
	if (rc == 0)
		gitlab_print_pipelines(&list);

	gitlab_pipelines_free(&list);

	return rc;
}

void
gitlab_print_pipelines(struct gitlab_pipeline_list const *const list)
{
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "UPDATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->pipelines_size) {
		printf("No pipelines\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gcli_tbl_add_row(table,
		                 list->pipelines[i].id,
		                 list->pipelines[i].status,
		                 list->pipelines[i].created_at,
		                 list->pipelines[i].updated_at,
		                 list->pipelines[i].ref);
	}

	gcli_tbl_end(table);
}

void
gitlab_print_jobs(struct gitlab_job_list const *const list)
{
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "STATUS",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FINISHED",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "RUNNERDESC", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->jobs_size) {
		printf("No jobs\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not initialize table");

	for (size_t i = 0; i < list->jobs_size; ++i) {
		gcli_tbl_add_row(table,
		                 list->jobs[i].id,
		                 list->jobs[i].name,
		                 list->jobs[i].status,
		                 list->jobs[i].started_at,
		                 list->jobs[i].finished_at,
		                 list->jobs[i].runner_description,
		                 list->jobs[i].ref);
	}

	gcli_tbl_end(table);
}

void
gitlab_print_job_status(struct gitlab_job const *const job)
{
	gcli_dict printer;

	printer = gcli_dict_begin();

	gcli_dict_add(printer,        "ID", 0, 0, "%"PRIid, job->id);
	gcli_dict_add_string(printer, "STATUS", GCLI_TBLCOL_STATECOLOURED, 0, job->status);
	gcli_dict_add_string(printer, "STAGE", 0, 0, job->stage);
	gcli_dict_add_string(printer, "NAME", GCLI_TBLCOL_BOLD, 0, job->name);
	gcli_dict_add_string(printer, "REF", GCLI_TBLCOL_COLOUREXPL, GCLI_COLOR_YELLOW, job->ref);
	gcli_dict_add_string(printer, "CREATED", 0, 0, job->created_at);
	gcli_dict_add_string(printer, "STARTED", 0, 0, job->started_at);
	gcli_dict_add_string(printer, "FINISHED", 0, 0, job->finished_at);
	gcli_dict_add(printer,        "DURATION", 0, 0, "%-.2lfs", job->duration);
	gcli_dict_add(printer,        "COVERAGE", 0, 0, "%.1lf%%", job->coverage);
	gcli_dict_add_string(printer, "RUNNER NAME", 0, 0, job->runner_name);
	gcli_dict_add_string(printer, "RUNNER DESCR", 0, 0, job->runner_description);

	gcli_dict_end(printer);
}

/* Pipeline actions */
struct pipeline_action_ctx {
	char const *const owner;
	char const *const repo;
	long const pipeline_id;

	int argc;
	char **argv;
};

static int
action_pipeline_jobs(struct pipeline_action_ctx *ctx)
{
	int rc = 0;
	struct gitlab_job_list jobs = {0};

	rc = gitlab_get_pipeline_jobs(g_clictx, ctx->owner, ctx->repo,
	                              ctx->pipeline_id, -1, &jobs);

	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get pipeline jobs: %s\n",
		        gcli_get_error(g_clictx));

		return EXIT_FAILURE;
	}

	gitlab_print_jobs(&jobs);
	gitlab_free_jobs(&jobs);

	return EXIT_SUCCESS;
}

static struct pipeline_action {
	char const *const name;
	int (*fn)(struct pipeline_action_ctx *ctx);
} const pipeline_actions[] = {
	{ .name = "jobs", .fn = action_pipeline_jobs },
};

static struct pipeline_action const *
find_pipeline_action(char const *const name)
{
	for (size_t i = 0; i < ARRAY_SIZE(pipeline_actions); ++i) {
		if (strcmp(name, pipeline_actions[i].name) == 0)
			return &pipeline_actions[i];
	}

	return NULL;
}

static int
handle_pipeline_actions(char const *const owner, char const *const repo,
                        long const pipeline_id, int argc, char *argv[])
{
	struct pipeline_action_ctx ctx = {
		.owner = owner,
		.repo = repo,
		.pipeline_id = pipeline_id,
		.argc = argc,
		.argv = argv,
	};

	if (!ctx.argc) {
		fprintf(stderr, "gcli: error: missing pipeline actions\n");
		usage();
		return EXIT_FAILURE;
	}

	while (ctx.argc) {
		char const *action_name;
		int exit_code;
		struct pipeline_action const *action;

		action_name = shift(&ctx.argc, &ctx.argv);
		action = find_pipeline_action(action_name);
		if (!action) {
			fprintf(stderr, "gcli: error: no such pipeline action: %s\n",
			        action_name);
			usage();
			return EXIT_FAILURE;
		}

		exit_code = action->fn(&ctx);
		if (exit_code)
			return exit_code;
	}

	return EXIT_SUCCESS;
}

/* Job related actions */
struct job_action_ctx {
	char const *owner, *repo;
	long const job_id;

	int argc;
	char **argv;
};

static int
action_job_status(struct job_action_ctx *ctx)
{
	struct gitlab_job job = {0};
	int rc = 0;

	rc = gitlab_get_job(g_clictx, ctx->owner, ctx->repo, ctx->job_id, &job);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get job status: %s\n",
		        gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	gitlab_print_job_status(&job);
	gitlab_free_job(&job);

	return EXIT_SUCCESS;
}

static int
action_job_log(struct job_action_ctx *ctx)
{
	int rc = gitlab_job_get_log(g_clictx, ctx->owner, ctx->repo, ctx->job_id, stdout);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get job log: %s\n",
		        gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
action_job_cancel(struct job_action_ctx *ctx)
{
	int rc = gitlab_job_cancel(g_clictx, ctx->owner, ctx->repo, ctx->job_id);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to cancel the job: %s\n",
		        gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
action_job_retry(struct job_action_ctx *ctx)
{
	int rc = gitlab_job_retry(g_clictx, ctx->owner, ctx->repo, ctx->job_id);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to retry the job: %s\n",
		        gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
action_job_artifacts(struct job_action_ctx *ctx)
{
	int rc = 0;
	char const *outfile = "artifacts.zip";

	if (ctx->argc && strcmp(ctx->argv[0], "-o") == 0) {
		if (ctx->argc < 2) {
			fprintf(stderr, "gcli: error: -o is missing the output filename\n");
			usage();
			return EXIT_FAILURE;
		}

		outfile = ctx->argv[1];
		ctx->argc -= 2;
		ctx->argv += 2;
	}

	rc = gitlab_job_download_artifacts(g_clictx, ctx->owner, ctx->repo,
	                                   ctx->job_id, outfile);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to download file: %s\n",
		        gcli_get_error(g_clictx));

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static struct job_action {
	char const *const name;                 /* Name on the cli */
	int (*fn)(struct job_action_ctx *ctx);  /* Function to be invoked for this action */
} const job_actions[] = {
	{ .name = "log",       .fn = action_job_log       },
	{ .name = "status",    .fn = action_job_status    },
	{ .name = "cancel",    .fn = action_job_cancel    },
	{ .name = "retry",     .fn = action_job_retry     },
	{ .name = "artifacts", .fn = action_job_artifacts },
};

static struct job_action const *
find_job_action(char const *const name)
{
	for (size_t i = 0; i < ARRAY_SIZE(job_actions); ++i) {
		if (strcmp(name, job_actions[i].name) == 0)
			return &job_actions[i];
	}

	return NULL;
}

static int
handle_job_actions(char const *const owner, char const *const repo,
                   long const job_id, int argc, char *argv[])
{
	struct job_action_ctx ctx = {
		.owner = owner,
		.repo = repo,
		.job_id = job_id,
		.argc = argc,
		.argv = argv,
	};

	/* Check if the user missed out on supplying actions */
	if (ctx.argc == 0) {
		fprintf(stderr, "gcli: error: no actions supplied\n");
		usage();
		exit(EXIT_FAILURE);
	}

	/* Parse and execute all the actions */
	while (ctx.argc) {
		int exit_code = 0;
		char const *action_name;
		struct job_action const *action;

		action_name = shift(&ctx.argc, &ctx.argv);
		action = find_job_action(action_name);

		if (!action) {
			fprintf(stderr, "gcli: error: unknown action '%s'\n", action_name);
			usage();
			return EXIT_FAILURE;
		}

		exit_code = action->fn(&ctx);
		if (exit_code)
			return exit_code;
	}

	return EXIT_SUCCESS;
}

static int
list_pipelines(char const *const owner, char const *const repo, int max)
{
	struct gitlab_pipeline_list list = {0};
	int rc = 0;

	rc = gitlab_get_pipelines(g_clictx, owner, repo, max, &list);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to get pipelines: %s\n",
		        gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	gitlab_print_pipelines(&list);
	gitlab_pipelines_free(&list);

	return EXIT_SUCCESS;
}

int
subcommand_pipelines(int argc, char *argv[])
{
	int ch = 0;
	char const *owner = NULL, *repo = NULL;
	int count = 30;
	long pipeline_id = -1;              /* pipeline id                           */
	long job_id = -1;              /* job id. these are mutually exclusive. */

	/* Parse options */
	const struct option options[] = {
		{.name = "repo",     .has_arg = required_argument, .flag = NULL, .val = 'r'},
		{.name = "owner",    .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "count",    .has_arg = required_argument, .flag = NULL, .val = 'c'},
		{.name = "pipeline", .has_arg = required_argument, .flag = NULL, .val = 'p'},
		{.name = "job",      .has_arg = required_argument, .flag = NULL, .val = 'j'},
		{0}
	};

	while ((ch = getopt_long(argc, argv, "+n:o:r:p:j:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse argument to -n");
		} break;
		case 'p': {
			char *endptr = NULL;
			pipeline_id = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse argument to -p");
			if (pipeline_id < 0) {
				errx(1, "gcli: error: pipeline id must be a positive number");
			}
		} break;
		case 'j': {
			char *endptr = NULL;
			job_id = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse argument to -j");
			if (job_id < 0) {
				errx(1, "gcli: error: job id must be a positive number");
			}
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (pipeline_id > 0 && job_id > 0) {
		fprintf(stderr, "gcli: error: -p and -j are mutually exclusive\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);

	/* Make sure we are actually talking about a gitlab remote because
	 * we might be incorrectly inferring it */
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITLAB)
		errx(1, "gcli: error: The pipelines subcommand only works for GitLab. "
		     "Use gcli -t gitlab ... to force a GitLab remote.");

	/* In case a Pipeline ID was specified handle its actions */
	if (pipeline_id >= 0)
		return handle_pipeline_actions(owner, repo, pipeline_id, argc, argv);

	/* A Job ID was specified */
	if (job_id >= 0)
		return handle_job_actions(owner, repo, job_id, argc, argv);

	/* Neither a Job id nor a pipeline ID was specified - list all
	 * pipelines instead */
	if (argc != 0) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	return list_pipelines(owner, repo, count);
}
