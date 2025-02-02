/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/actions.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/open.h>
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
	fprintf(stderr, "  all                      Show status of this pipeline (including jobs and children)\n");
	fprintf(stderr, "  children                 Print the list of child pipelines\n");
	fprintf(stderr, "  jobs                     Print the list of jobs of this pipeline\n");
	fprintf(stderr, "  open                     Open the pipeline in a web browser\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "JOB ACTIONS:\n");
	fprintf(stderr, "  status                   Display status information\n");
	fprintf(stderr, "  artifacts [-o filename]  Download a zip archive of the artifacts of the given job\n");
	fprintf(stderr, "                           (default output filename: artifacts.zip)\n");
	fprintf(stderr, "  log                      Display job log\n");
	fprintf(stderr, "  cancel                   Cancel the job\n");
	fprintf(stderr, "  retry                    Retry the given job\n");
	fprintf(stderr, "  open                     Open the job in a web browser\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gitlab_print_pipelines(struct gitlab_pipeline_list const *const list)
{
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "UPDATED", .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "NAME",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
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
		                 list->pipelines[i].name,
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
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "FINISHED",   .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
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

	gcli_dict_add(printer,           "ID", 0, 0, "%"PRIid, job->id);
	gcli_dict_add_string(printer,    "STATUS", GCLI_TBLCOL_STATECOLOURED, 0, job->status);
	gcli_dict_add_string(printer,    "STAGE", 0, 0, job->stage);
	gcli_dict_add_string(printer,    "NAME", GCLI_TBLCOL_BOLD, 0, job->name);
	gcli_dict_add_string(printer,    "REF", GCLI_TBLCOL_COLOUREXPL, GCLI_COLOR_YELLOW, job->ref);
	gcli_dict_add_timestamp(printer, "CREATED", 0, 0, job->created_at);
	gcli_dict_add_timestamp(printer, "STARTED", 0, 0, job->started_at);
	gcli_dict_add_timestamp(printer, "FINISHED", 0, 0, job->finished_at);
	gcli_dict_add(printer,           "DURATION", 0, 0, "%-.2lfs", job->duration);
	gcli_dict_add(printer,           "COVERAGE", 0, 0, "%.1lf%%", job->coverage);
	gcli_dict_add_string(printer,    "RUNNER NAME", 0, 0, job->runner_name);
	gcli_dict_add_string(printer,    "RUNNER DESCR", 0, 0, job->runner_description);

	gcli_dict_end(printer);
}

void
gitlab_print_pipeline(struct gitlab_pipeline const *const pipeline)
{
	gcli_dict printer;

	printer = gcli_dict_begin();

	gcli_dict_add(printer,           "ID", 0, 0, "%"PRIid, pipeline->id);
	gcli_dict_add_string(printer,    "NAME", 0, 0, pipeline->name ? pipeline->name : "N/A");
	gcli_dict_add_string(printer,    "STATUS", GCLI_TBLCOL_STATECOLOURED, 0, pipeline->status);
	gcli_dict_add_timestamp(printer, "CREATED", 0, 0, pipeline->created_at);
	gcli_dict_add_timestamp(printer, "UPDATED", 0, 0, pipeline->updated_at);
	gcli_dict_add_string(printer,    "REF", GCLI_TBLCOL_COLOUREXPL, GCLI_COLOR_YELLOW, pipeline->ref);
	gcli_dict_add_string(printer,    "SHA", GCLI_TBLCOL_COLOUREXPL, GCLI_COLOR_YELLOW, pipeline->sha);
	gcli_dict_add_string(printer,    "SOURCE", 0, 0, pipeline->source);

	gcli_dict_end(printer);
}

static int
action_pipeline_status(struct gcli_path const *path,
                       struct gitlab_pipeline *pipeline,
                       int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gitlab_print_pipeline(pipeline);
	return GCLI_EX_OK;
}

static int
action_pipeline_jobs(struct gcli_path const *path,
                     struct gitlab_pipeline *pipeline,
                     int *argc, char **argv[])
{
	int rc = 0;
	struct gitlab_job_list jobs = {0};

	(void) pipeline;
	(void) argc;
	(void) argv;

	rc = gitlab_get_pipeline_jobs(g_clictx, path, -1, &jobs);

	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get pipeline jobs: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	gitlab_print_jobs(&jobs);
	gitlab_free_jobs(&jobs);

	return GCLI_EX_OK;
}

static int
action_pipeline_children(struct gcli_path const *path,
                         struct gitlab_pipeline *pipeline,
                         int *argc, char **argv[])
{
	int rc = 0;
	struct gitlab_pipeline_list children = {0};

	(void) pipeline;
	(void) argc;
	(void) argv;

	rc = gitlab_get_pipeline_children(g_clictx, path, -1, &children);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get pipeline children: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	gitlab_print_pipelines(&children);
	gitlab_pipelines_free(&children);

	return GCLI_EX_OK;
}

static int
action_pipeline_all(struct gcli_path const *path,
                    struct gitlab_pipeline *pipeline,
                    int *argc, char **argv[])
{
	int rc = 0;

	rc = action_pipeline_status(path, pipeline, argc, argv);
	if (rc)
		return rc;

	fprintf(stdout, "\n");

	fprintf(stdout, "JOBS\n");
	rc = action_pipeline_jobs(path, pipeline, argc, argv);
	if (rc)
		return rc;

	fprintf(stdout, "\n");

	fprintf(stdout, "CHILDREN\n");
	rc = action_pipeline_children(path, pipeline, argc, argv);

	return rc;
}

static int
action_pipeline_open(struct gcli_path const *path,
                     struct gitlab_pipeline *pipeline,
                     int *argc, char **argv[])
{
	int rc;

	(void) path;
	(void) argc;
	(void) argv;

	rc = gcli_cmd_open_url(pipeline->web_url);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to open url\n");
		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static struct gcli_cmd_actions const pipeline_actions = {
	.fetch_item = (gcli_cmd_action_fetcher)gitlab_get_pipeline,
	.free_item = (gcli_cmd_action_freeer)gitlab_pipeline_free,
	.item_size = sizeof(struct gitlab_pipeline),

	.defs = {
		{
			.name = "all",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_pipeline_all
		},
		{
			.name = "status",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_pipeline_status
		},
		{
			.name = "jobs",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_pipeline_jobs
		},
		{
			.name = "children",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_pipeline_children
		},
		{
			.name = "open",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_pipeline_open
		},
		{0},
	},
};

static int
handle_pipeline_actions(struct gcli_path const *const pipeline_path,
                        int argc, char *argv[])
{
	int rc = 0;

	if (argc == 0) {
		fprintf(stderr, "gcli: error: missing pipeline actions\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = gcli_cmd_actions_handle(&pipeline_actions, pipeline_path, &argc, &argv);
	if (rc == GCLI_EX_USAGE)
		usage();

	return !!rc;
}

/* *****************************
 * Gitlab Jobs
 * *****************************/
static int
action_job_status(struct gcli_path const *const path,
                  struct gitlab_job const *const job,
                  int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gitlab_print_job_status(job);

	return GCLI_EX_OK;
}

static int
action_job_log(struct gcli_path const *const path,
               struct gitlab_job const *const job,
               int *argc, char **argv[])
{
	int rc = 0;

	(void) job;
	(void) argc;
	(void) argv;

	rc = gitlab_job_get_log(g_clictx, path, stdout);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get job log: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_job_cancel(struct gcli_path const *const path,
                  struct gitlab_job const *const job,
                  int *argc, char **argv[])
{
	int rc = 0;

	(void) job;
	(void) argc;
	(void) argv;

	rc = gitlab_job_cancel(g_clictx, path);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to cancel the job: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_job_retry(struct gcli_path const *const path,
                 struct gitlab_job const *const job,
                 int *argc, char **argv[])
{
	int rc = 0;

	(void) job;
	(void) argc;
	(void) argv;

	rc = gitlab_job_retry(g_clictx, path);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to retry the job: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_job_artifacts(struct gcli_path const *const path,
                     struct gitlab_job const *const job,
                     int *argc, char **argv[])
{
	int rc = 0;
	char const *outfile = "artifacts.zip";

	(void) job;

	if (*argc > 2 && strcmp((*argv)[1], "-o") == 0) {
		if (*argc < 3) {
			fprintf(stderr, "gcli: error: -o is missing the output filename\n");
			usage();
			return EXIT_FAILURE;
		}

		outfile = (*argv)[2];
		*argc -= 2;
		*argv += 2;
	}

	rc = gitlab_job_download_artifacts(g_clictx, path, outfile);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to download file: %s\n",
		        gcli_get_error(g_clictx));

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
action_job_open(struct gcli_path const *path,
                struct gitlab_job *job,
                int *argc, char **argv[])
{
	int rc;

	(void) path;
	(void) argc;
	(void) argv;

	rc = gcli_cmd_open_url(job->web_url);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to open url\n");
		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static struct gcli_cmd_actions job_actions = {
	.fetch_item = (gcli_cmd_action_fetcher)gitlab_get_job,
	.free_item = (gcli_cmd_action_freeer)gitlab_free_job,
	.item_size = sizeof(struct gitlab_job),

	.defs = {
		{
			.name = "log",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_job_log,
		},
		{
			.name = "status",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_job_status,
		},
		{
			.name = "cancel",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_job_cancel,
		},
		{
			.name = "retry",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_job_retry,
		},
		{
			.name = "artifacts",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_job_artifacts,
		},
		{
			.name = "open",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_job_open,
		},
		{0},
	},
};

static int
handle_job_actions(struct gcli_path const *const job_path,
                   int argc, char *argv[])
{
	int rc = 0;

	/* Check if the user missed out on supplying actions */
	if (argc == 0) {
		fprintf(stderr, "gcli: error: no actions supplied\n");
		usage();
		return GCLI_EX_USAGE;
	}

	rc = gcli_cmd_actions_handle(&job_actions, job_path, &argc, &argv);
	if (rc == GCLI_EX_USAGE)
		usage();

	return !!rc;
}

static int
list_pipelines(struct gcli_path const *const path, int max)
{
	struct gitlab_pipeline_list list = {0};
	int rc = 0;

	rc = gitlab_get_pipelines(g_clictx, path, max, &list);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to get pipelines: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	gitlab_print_pipelines(&list);
	gitlab_pipelines_free(&list);

	return GCLI_EX_OK;
}

int
subcommand_pipelines(int argc, char *argv[])
{
	int ch = 0, count = 30, pflag = 0, jflag = 0;
	struct gcli_path path = {0};

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
			path.data.as_default.owner = optarg;
			break;
		case 'r':
			path.data.as_default.owner = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg))) {
				fprintf(stderr, "gcli: error: cannot parse argument to -n\n");
				return EXIT_FAILURE;
			}
		} break;
		case 'p': {
			char *endptr = NULL;
			path.data.as_default.id = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg))) {
				fprintf(stderr, "gcli: error: cannot parse argument to -p\n");
				return EXIT_FAILURE;
			}

			pflag = 1;
		} break;
		case 'j': {
			char *endptr = NULL;
			path.data.as_default.id = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg))) {
				fprintf(stderr, "gcli: error: cannot parse argument to -j\n");
				return EXIT_FAILURE;
			}

			jflag = 1;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (pflag && jflag) {
		fprintf(stderr, "gcli: error: -p and -j are mutually exclusive\n");
		usage();
		return EXIT_FAILURE;
	}

	check_path(&path);

	/* Make sure we are actually talking about a gitlab remote because
	 * we might be incorrectly inferring it */
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITLAB) {
		fprintf(stderr, "gcli: error: The pipelines subcommand only works for GitLab. "
		     "Use gcli -t gitlab ... to force a GitLab remote.\n");

		return EXIT_FAILURE;
	}

	/* In case a Pipeline ID was specified handle its actions */
	if (pflag)
		return handle_pipeline_actions(&path, argc, argv);

	/* A Job ID was specified */
	if (jflag)
		return handle_job_actions(&path, argc, argv);

	/* Neither a Job id nor a pipeline ID was specified - list all
	 * pipelines instead */
	if (argc != 0) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	return list_pipelines(&path, count);
}
