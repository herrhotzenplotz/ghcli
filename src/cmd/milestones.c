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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/issues.h>
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

static int handle_milestone_actions(int argc, char *argv[],
                                    struct gcli_path const *path);


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

static void
ensure_milestone(struct gcli_path const *const path,
                 int *const fetched_milestone,
                 struct gcli_milestone *const milestone)
{
	int rc;

	if (*fetched_milestone)
		return;

	rc = gcli_get_milestone(g_clictx, path, milestone);
	if (rc < 0)
		errx(1, "gcli: error: could not get milestone: %s",
		     gcli_get_error(g_clictx));

	*fetched_milestone = 1;
}

static int
handle_milestone_actions(int argc, char *argv[],
                         struct gcli_path const *const path)
{
	struct gcli_milestone milestone = {0};
	int fetched_milestone = 0;

	/* Check if the user missed out on supplying actions */
	if (argc == 0) {
		fprintf(stderr, "gcli: error: no actions supplied\n");
		usage();
		exit(EXIT_FAILURE);
	}


	/* Iterate over all the actions */
	while (argc) {
		/* Read in action */
		char const *action = shift(&argc, &argv);

		/* Dispatch */
		if (strcmp(action, "all") == 0) {
			int rc = 0;

			struct gcli_issue_list issues = {0};

			ensure_milestone(path, &fetched_milestone, &milestone);

			gcli_print_milestone(&milestone);
			rc = gcli_milestone_get_issues(g_clictx, path, &issues);

			if (rc < 0) {
				errx(1, "gcli: error: failed to fetch issues: %s",
				     gcli_get_error(g_clictx));
			}

			printf("\nISSUES:\n");
			gcli_print_issues(0, &issues, -1);
			gcli_issues_free(&issues);

		} else if (strcmp(action, "issues") == 0) {
			int rc = 0;
			struct gcli_issue_list issues = {0};

			/* Fetch list of issues associated with milestone */
			rc = gcli_milestone_get_issues(g_clictx, path, &issues);
			if (rc < 0) {
				errx(1, "gcli: error: failed to fetch issues: %s",
				     gcli_get_error(g_clictx));
			}

			/* Print them as a table */
			gcli_print_issues(0, &issues, -1);

			/* Cleanup */
			gcli_issues_free(&issues);

		} else if (strcmp(action, "status") == 0) {

			/* Make sure we have the milestone data */
			ensure_milestone(path, &fetched_milestone, &milestone);

			/* Print meta */
			gcli_print_milestone(&milestone);
		} else if (strcmp(action, "delete") == 0) {

			/* Delete the milestone */
			if (gcli_delete_milestone(g_clictx, path) < 0) {
				errx(1, "gcli: error: could not delete milestone: %s",
				     gcli_get_error(g_clictx));
			}

		} else if (strcmp(action, "set-duedate") == 0) {

			char *new_date = NULL;
			int rc = 0;

			/* grab the the date that the user provided */
			if (!argc)
				errx(1, "gcli: error: missing date for set-duedate");

			new_date = shift(&argc, &argv);

			/* Do it! */
			rc = gcli_milestone_set_duedate(g_clictx, path, new_date);
			if (rc < 0) {
				errx(1, "gcli: error: could not update milestone due date: %s",
				     gcli_get_error(g_clictx));;
			}

		} else {

			/* We don't know of the action - maybe a syntax error or
			 * trailing garbage. Error out in this case. */
			fprintf(stderr, "gcli: error: unknown action %s\n", action);
			usage();
			return EXIT_FAILURE;
		}


		/* Print a blank line if we are not at the end */
		if (argc)
			putchar('\n');
	}

	/* Cleanup the milestone if we ever fetched it */
	if (fetched_milestone)
		gcli_free_milestone(&milestone);

	return EXIT_SUCCESS;
}
