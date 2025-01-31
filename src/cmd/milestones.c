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

#include <config.h>

#include <gcli/cmd/actions.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/issues.h>
#include <gcli/cmd/open.h>
#include <gcli/cmd/table.h>

#include <gcli/forges.h>
#include <gcli/milestones.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli milestones [-o owner -r repo]\n");
	fprintf(stderr, "       gcli milestones create [-o owner -r repo] -t title [-d description]\n");
	fprintf(stderr, "       gcli milestones [-o owner -r repo] -i milestone actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner             The repository owner\n");
	fprintf(stderr, "  -r repo              The repository name\n");
	fprintf(stderr, "  -i milestone         Run actions for the given milestone id\n");
	fprintf(stderr, "  -t title             Title of the milestone to create\n");
	fprintf(stderr, "  -d description       Description the milestone to create\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  all                  Display both status information and issues for the milestone\n");
	fprintf(stderr, "  status               Display general status information about the milestone\n");
	fprintf(stderr, "  issues               List issues associated with the milestone\n");
	fprintf(stderr, "  set-duedate <date>   Set due date \n");
	fprintf(stderr, "  delete               Delete this milestone\n");
	fprintf(stderr, "  open                 Open this milestone in a web browser\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_print_milestones(struct gcli_milestone_list const *const list, int max)
{
	size_t n;
	gcli_tbl tbl;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->milestones_size) {
		puts("No milestones");
		return;
	}

	tbl = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!tbl)
		errx(1, "gcli: error: could not init table printer");

	if (max < 0 || (size_t)(max) > list->milestones_size)
		n = list->milestones_size;
	else
		n = max;

	for (size_t i = 0; i < n; ++i) {
		gcli_tbl_add_row(tbl,
		                 list->milestones[i].id,
		                 list->milestones[i].state,
		                 list->milestones[i].created_at,
		                 list->milestones[i].title);
	}

	gcli_tbl_end(tbl);
}

void
gcli_print_milestone(struct gcli_milestone const *const milestone)
{
	gcli_dict dict;
	uint32_t const quirks = gcli_forge(g_clictx)->milestone_quirks;

	dict = gcli_dict_begin();
	gcli_dict_add(dict,           "ID", 0, 0, "%"PRIid, milestone->id);
	gcli_dict_add_string(dict,    "TITLE", 0, 0, milestone->title);
	gcli_dict_add_string(dict,    "STATE", GCLI_TBLCOL_STATECOLOURED, 0, milestone->state);
	gcli_dict_add_timestamp(dict, "CREATED", 0, 0, milestone->created_at);
	gcli_dict_add_timestamp(dict, "UPDATED", 0, 0, milestone->created_at);

	if ((quirks & GCLI_MILESTONE_QUIRKS_DUEDATE) == 0)
		gcli_dict_add_timestamp(dict, "DUE", 0, 0, milestone->due_date);

	if ((quirks & GCLI_MILESTONE_QUIRKS_EXPIRED) == 0)
		gcli_dict_add_string(dict, "EXPIRED", 0, 0, sn_bool_yesno(milestone->expired));

	if ((quirks & GCLI_MILESTONE_QUIRKS_NISSUES) == 0) {
		gcli_dict_add(dict, "OPEN ISSUES", 0, 0, "%d", milestone->open_issues);
		gcli_dict_add(dict, "CLOSED ISSUES", 0, 0, "%d", milestone->closed_issues);
	}

	gcli_dict_end(dict);

	if (milestone->description && strlen(milestone->description)) {
		printf("\nDESCRIPTION:\n");
		gcli_pretty_print(milestone->description, 4, 80, stdout);
	}
}

static int
action_milestone_all(struct gcli_path const *const path,
                     struct gcli_milestone const *item,
                     int *argc, char **argv[])
{
	struct gcli_issue_list issues = {0};
	int rc = 0;

	(void) argc;
	(void) argv;

	gcli_print_milestone(item);
	rc = gcli_milestone_get_issues(g_clictx, path, &issues);

	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to fetch issues: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	printf("\nISSUES:\n");
	gcli_print_issues(0, &issues, -1);
	gcli_issues_free(&issues);

	return GCLI_EX_OK;
}

static int
action_milestone_issues(struct gcli_path const *const path,
                        struct gcli_milestone const *item,
                        int *argc, char **argv[])
{
	int rc = 0;
	struct gcli_issue_list issues = {0};

	(void) item;
	(void) argc;
	(void) argv;

	/* Fetch list of issues associated with milestone */
	rc = gcli_milestone_get_issues(g_clictx, path, &issues);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to get issues: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	/* Print them as a table */
	gcli_print_issues(0, &issues, -1);

	/* Cleanup */
	gcli_issues_free(&issues);

	return GCLI_EX_OK;
}

static int
action_milestone_status(struct gcli_path const *const path,
                        struct gcli_milestone const *const milestone,
                        int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_print_milestone(milestone);
	return GCLI_EX_OK;
}

static int
action_milestone_delete(struct gcli_path const *const path,
                        struct gcli_milestone const *const milestone,
                        int *argc, char **argv[])
{
	int rc;

	(void) milestone;
	(void) argc;
	(void) argv;

	/* Delete the milestone */
	rc = gcli_delete_milestone(g_clictx, path);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: could not delete milestone: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_milestone_set_duedate(struct gcli_path const *const path,
                             struct gcli_milestone const *const milestone,
                             int *argc, char **argv[])
{
	char *new_date = NULL;
	int rc = 0;

	(void) milestone;

	/* grab the the date that the user provided */
	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing date for set-duedate\n");
		return GCLI_EX_USAGE;
	}

	shift(argc, argv);
	new_date = (*argv)[0];

	/* Do it! */
	rc = gcli_milestone_set_duedate(g_clictx, path, new_date);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: could not update milestone due date: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_milestone_open(struct gcli_path const *const path,
                      struct gcli_milestone const *const milestone,
                      int *argc, char **argv[])
{
	int rc;

	(void) path;
	(void) argc;
	(void) argv;

	rc = gcli_cmd_open_url(milestone->web_url);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to open url\n");
		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

struct gcli_cmd_actions milestone_actions = {
	.fetch_item = (gcli_cmd_action_fetcher)gcli_get_milestone,
	.free_item = (gcli_cmd_action_freeer)gcli_free_milestone,
	.item_size = sizeof(struct gcli_milestone),

	.defs = {
		{
			.name = "all",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_milestone_all,
		},
		{
			.name = "issues",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_milestone_issues,
		},
		{
			.name = "status",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_milestone_status,
		},
		{
			.name = "delete",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_milestone_delete,
		},
		{
			.name = "set-duedate",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler)action_milestone_set_duedate,
		},
		{
			.name = "open",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler)action_milestone_open,
		},
		{0},
	},
};

static int
handle_milestone_actions(int argc, char *argv[],
                         struct gcli_path const *const path)
{
	int rc = 0;

	rc = gcli_cmd_actions_handle(&milestone_actions, path, &argc, &argv);
	if (rc == GCLI_EX_USAGE) {
		usage();
	}

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int
subcommand_milestone_create(int argc, char *argv[])
{
	int ch;
	struct gcli_milestone_create_args args = {0};
	struct option const options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "title",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 't' },
		{ .name  = "description",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'd' },
	};

	/* Read in options */
	while ((ch = getopt_long(argc, argv, "+o:r:t:d:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			args.owner = optarg;
			break;
		case 'r':
			args.repo = optarg;
			break;
		case 't':
			args.title = optarg;
			break;
		case 'd':
			args.description = optarg;
			break;
		default:
			usage();
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	/* sanity check argumets */
	if (argc)
		errx(1, "gcli: error: stray arguments");

	/* make sure both are set or deduce them */
	check_owner_and_repo(&args.owner, &args.repo);

	/* enforce the user to at least provide a title */
	if (!args.title)
		errx(1, "gcli: error: missing milestone title");

	/* actually create the milestone */
	if (gcli_create_milestone(g_clictx, &args) < 0)
		errx(1, "gcli: error: could not create milestone: %s",
		     gcli_get_error(g_clictx));

	return 0;
}

int
subcommand_milestones(int argc, char *argv[])
{
	int ch, rc, max = 30;
	struct gcli_path path = {0};
	struct option const options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "count",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'n' },
		{ .name = "id",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'i' },
	};

	/* detect whether we wanna create a milestone */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_milestone_create(argc, argv);
	}

	/* Proceed with fetching information */
	while ((ch = getopt_long(argc, argv, "+o:r:n:i:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			path.data.as_default.owner = optarg;
		} break;
		case 'r': {
			path.data.as_default.repo = optarg;
		} break;
		case 'n': {
			char *endptr;
			max = strtol(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "gcli: error: cannot parse milestone count");
		} break;
		case 'i': {
			char *endptr;
			path.data.as_default.id = strtoul(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "gcli: error: cannot parse milestone id");
		} break;
		default: {
			usage();
			return 1;
		}
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&path);

	if (path.data.as_default.id == 0) {
		struct gcli_milestone_list list = {0};

		rc = gcli_get_milestones(g_clictx, &path, max, &list);
		if (rc < 0) {
			errx(1, "gcli: error: cannot get list of milestones: %s",
			     gcli_get_error(g_clictx));
		}

		gcli_print_milestones(&list, max);

		gcli_free_milestones(&list);

		return 0;
	}

	return handle_milestone_actions(argc, argv, &path);
}
