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
#include <gcli/cmd/comment.h>
#include <gcli/cmd/editor.h>
#include <gcli/cmd/interactive.h>
#include <gcli/cmd/table.h>

#include <gcli/comments.h>
#include <gcli/forges.h>
#include <gcli/issues.h>

#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli issues create [-o owner -r repo] [-y] [-R reviewer] [title...]\n");
	fprintf(stderr, "       gcli issues [-o owner -r repo] [-a] [-n number] [-A author] [-L label]\n");
	fprintf(stderr, "                   [-M milestone] [-s] [search query...]\n");
	fprintf(stderr, "       gcli issues [-o owner -r repo] -i issue actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner           The repository owner\n");
	fprintf(stderr, "  -r repo            The repository name\n");
	fprintf(stderr, "  -y                 Do not ask for confirmation.\n");
	fprintf(stderr, "  -A author          Only print issues by the given author\n");
	fprintf(stderr, "  -L label           Filter issues by the given label\n");
	fprintf(stderr, "  -M milestone       Filter issues by the given milestone\n");
	fprintf(stderr, "  -a                 Fetch everything including closed issues \n");
	fprintf(stderr, "  -s                 Print (sort) in reverse order\n");
	fprintf(stderr, "  -n number          Number of issues to fetch (-1 = everything)\n");
	fprintf(stderr, "  -i issue           ID of issue to perform actions on\n");
	fprintf(stderr, "  -R reviewer        Mark a person as a reviewer for the created PR\n");
	fprintf(stderr, "                     Can be specified more than once.\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  all                Display both status and and op\n");
	fprintf(stderr, "  status             Display status information\n");
	fprintf(stderr, "  op                 Display original post\n");
	fprintf(stderr, "  comments           Display comments\n");
	fprintf(stderr, "  close              Close the issue\n");
	fprintf(stderr, "  reopen             Reopen a closed issue\n");
	fprintf(stderr, "  assign <user>      Assign the issue to the given user\n");
	fprintf(stderr, "  labels ...         Add or remove labels:\n");
	fprintf(stderr, "                        add <name>\n");
	fprintf(stderr, "                        remove <name>\n");
	fprintf(stderr, "  milestone <id>     Assign this issue to the given milestone\n");
	fprintf(stderr, "  milestone -d       Clear the assigned milestone of the given issue\n");
	fprintf(stderr, "  notes              Alias for comments\n");
	fprintf(stderr, "  title <new-title>  Change the title of the issue\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_print_issues(enum gcli_output_flags const flags,
                  struct gcli_issue_list const *const list, int const max)
{
	int n, pruned = 0;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "NUMBER", .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NOTES",  .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "TITLE",  .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->issues_size == 0) {
		puts("No issues");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: could not init table printer");

	/* Determine the correct number of items to print */
	if (max < 0 || (size_t)(max) > list->issues_size)
		n = list->issues_size;
	else
		n = max;

	/* Iterate depending on the output order */
	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i) {
			if (!list->issues[n - 1 - 1].is_pr) {
				gcli_tbl_add_row(table,
				                 list->issues[n - i - 1].number,
				                 list->issues[n - i - 1].comments,
				                 list->issues[n - i - 1].state,
				                 list->issues[n - i - 1].title);
			} else {
				pruned++;
			}
		}
	} else {
		for (int i = 0; i < n; ++i) {
			if (!list->issues[i].is_pr) {
				gcli_tbl_add_row(table,
				                 list->issues[i].number,
				                 list->issues[i].comments,
				                 list->issues[i].state,
				                 list->issues[i].title);
			} else {
				pruned++;
			}
		}
	}

	/* Dump the table */
	gcli_tbl_end(table);

	/* Inform the user that we pruned pull requests from the output */
	if (pruned && sn_getverbosity() != VERBOSITY_QUIET)
		fprintf(stderr, "info: %d pull requests pruned\n", pruned);
}

void
gcli_issue_print_summary(struct gcli_issue const *const it)
{
	gcli_dict dict;
	uint32_t const quirks = gcli_forge(g_clictx)->issue_quirks;

	dict = gcli_dict_begin();

	gcli_dict_add(dict, "NUMBER", 0, 0, "%"PRIid, it->number);
	gcli_dict_add(dict, "TITLE", 0, 0, "%s", it->title);

	gcli_dict_add_timestamp(dict, "CREATED", 0, 0, it->created_at);

	if ((quirks & GCLI_ISSUE_QUIRKS_PROD_COMP) == 0) {
		gcli_dict_add(dict, "PRODUCT", 0, 0, "%s", it->product);
		gcli_dict_add(dict, "COMPONENT", 0, 0, "%s", it->component);
	}

	gcli_dict_add(dict, "AUTHOR",  GCLI_TBLCOL_BOLD, 0,
	              "%s", it->author);
	gcli_dict_add(dict, "STATE", GCLI_TBLCOL_STATECOLOURED, 0,
	              "%s", it->state);

	if ((quirks & GCLI_ISSUE_QUIRKS_URL) == 0 && !sn_strempty(it->url))
		gcli_dict_add(dict, "URL", 0, 0, "%s", it->url);

	if ((quirks & GCLI_ISSUE_QUIRKS_COMMENTS) == 0)
		gcli_dict_add(dict, "COMMENTS", 0, 0, "%d", it->comments);

	if ((quirks & GCLI_ISSUE_QUIRKS_LOCKED) == 0)
		gcli_dict_add(dict, "LOCKED", 0, 0, "%s", sn_bool_yesno(it->locked));

	if (!sn_strempty(it->milestone))
		gcli_dict_add(dict, "MILESTONE", 0, 0, "%s", it->milestone);

	if (it->labels_size) {
		gcli_dict_add_string_list(dict, "LABELS",
		                          (char const *const *)it->labels,
		                          it->labels_size);
	} else {
		gcli_dict_add(dict, "LABELS", 0, 0, "none");
	}

	if (it->assignees_size) {
		gcli_dict_add_string_list(dict, "ASSIGNEES",
		                          (char const *const *)it->assignees,
		                          it->assignees_size);
	} else {
		gcli_dict_add(dict, "ASSIGNEES", 0, 0, "none");
	}

	/* Dump the dictionary */
	gcli_dict_end(dict);
}

void
gcli_issue_print_op(struct gcli_issue const *const it)
{
	if (it->body)
		gcli_pretty_print(it->body, 4, 80, stdout);
}

static void
issue_init_user_file(struct gcli_ctx *ctx, FILE *stream, void *_opts)
{
	(void) ctx;
	struct gcli_submit_issue_options *opts = _opts;
	fprintf(
		stream,
		"! ISSUE TITLE : %s\n"
		"! Enter issue description above.\n"
		"! All lines starting with '!' will be discarded.\n"
		"!\n"
		"! vim: ft=markdown\n",
		opts->title);
}

static char *
gcli_issue_get_user_message(struct gcli_submit_issue_options *opts)
{
	return gcli_editor_get_user_message(g_clictx, issue_init_user_file, opts);
}

static int
create_issue(struct gcli_submit_issue_options *opts, int always_yes)
{
	int rc;

	opts->body = gcli_issue_get_user_message(opts);

	printf("The following issue will be created:\n"
	       "\n"
	       "TITLE   : %s\n"
	       "OWNER   : %s\n"
	       "REPO    : %s\n"
	       "MESSAGE :\n",
	       opts->title, opts->owner, opts->repo);

	if (opts->body)
		gcli_pretty_print(opts->body, 4, 80, stdout);
	else
		puts("No message");

	if (!always_yes) {
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "gcli: Submission aborted.");
	}

	rc = gcli_issue_submit(g_clictx, opts);

	free(opts->body);
	free(opts->body);

	return rc;
}

static int
subcommand_issue_create_interactive(struct gcli_submit_issue_options *const opts)
{
	char const *deflt_owner = NULL, *deflt_repo = NULL;
	int rc = 0;

	gcli_config_get_repo(g_clictx, &deflt_owner, &deflt_repo);

	if (!opts->owner)
		opts->owner = gcli_cmd_prompt("Owner", deflt_owner);

	if (!opts->repo)
		opts->repo = gcli_cmd_prompt("Repository", deflt_repo);

	opts->title = gcli_cmd_prompt("Title", GCLI_PROMPT_RESULT_MANDATORY);

	rc = create_issue(opts, false);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: %s\n", gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
parse_submit_issue_option(struct gcli_submit_issue_options *opts)
{
	char *hd = strdup(optarg);
	char *key = hd;
	char *value = NULL;

	hd = strchr(hd, '=');
	if (hd == NULL || *hd != '=') {
		fprintf(stderr, "gcli: -O expects a key-value-pair as key=value\n");
		return -1;
	}

	*hd++ = '\0';
	value = strdup(hd); /* make key and value separate allocations */

	gcli_nvlist_append(&opts->extra, key, value);

	return 0;
}

static int
subcommand_issue_create(int argc, char *argv[])
{
	int ch;
	struct gcli_submit_issue_options opts = {0};
	int always_yes = 0;

	if (gcli_nvlist_init(&opts.extra) < 0) {
		fprintf(stderr, "gcli: failed to init nvlist: %s\n",
		        strerror(errno));
		return EXIT_FAILURE;
	}

	const struct option options[] = {
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "o:r:O:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			opts.owner = optarg;
			break;
		case 'r':
			opts.repo = optarg;
			break;
		case 'y':
			always_yes = 1;
			break;
		case 'O': {
			int rc = parse_submit_issue_option(&opts);
			if (rc < 0)
				return EXIT_FAILURE;
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		return subcommand_issue_create_interactive(&opts);

	check_owner_and_repo(&opts.owner, &opts.repo);

	if (argc != 1) {
		fprintf(stderr, "gcli: error: Expected one argument for issue title\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.title = argv[0];

	if (create_issue(&opts, always_yes) < 0)
		errx(1, "gcli: error: failed to submit issue: %s",
		     gcli_get_error(g_clictx));

	gcli_nvlist_free(&opts.extra);

	return EXIT_SUCCESS;
}

static inline int handle_issues_actions(int argc, char *argv[],
                                        struct gcli_path const *path);

int
subcommand_issues(int argc, char *argv[])
{
	struct gcli_issue_list list = {0};
	struct gcli_path path = {0};
	char *endptr = NULL;
	int ch = 0, n = 30;
	struct gcli_issue_fetch_details details = {0};
	enum gcli_output_flags flags = 0;

	/* detect whether we wanna create an issue */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_issue_create(argc, argv);
	}

	const struct option options[] = {
		{ .name    = "all",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'a' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "id",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "author",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'A', },
		{ .name    = "label",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'L',
		},
		{ .name    = "milestone",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'M',
		},
		{0},
	};

	/* parse options */
	while ((ch = getopt_long(argc, argv, "+sn:o:r:i:aA:L:M:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			path.data.as_default.owner = optarg;
			break;
		case 'r':
			path.data.as_default.repo = optarg;
			break;
		case 'i': {
			path.data.as_default.id = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse issue number");

			if (path.data.as_default.id == 0)
				errx(1, "gcli: error: issue number is out of range");
		} break;
		case 'n': {
			n = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse issue count");

			if (n < -1)
				errx(1, "gcli: error: issue count is out of range");

			if (n == 0)
				errx(1, "gcli: error: issue count must not be zero");
		} break;
		case 'a':
			details.all = true;
			break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case 'A': {
			details.author = optarg;
		} break;
		case 'L': {
			details.label = optarg;
		} break;
		case 'M': {
			details.milestone = optarg;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&path);

	/* No issue number was given, so list all open issues */
	if (path.data.as_default.id == 0) {
		/* Prepare search term if specified */
		if (argc)
			details.search_term = sn_join_with((char const *const *)argv, argc, " ");

		if (gcli_issues_search(g_clictx, &path, &details, n, &list) < 0)
			errx(1, "gcli: error: could not get issues: %s", gcli_get_error(g_clictx));

		gcli_print_issues(flags, &list, n);

		gcli_issues_free(&list);
		return EXIT_SUCCESS;
	}

	/* require -a to not be set */
	if (details.all) {
		fprintf(stderr, "gcli: error: -a cannot be combined with operations on "
		        "an issue\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Handle all the actions */
	return handle_issues_actions(argc, argv, &path);
}

static int
action_labels(struct gcli_path const *const issue_path,
              struct gcli_issue const *const issue,
              int *argc, char **argv[])
{
	char const **add_labels = NULL;
	size_t add_labels_size = 0;
	char const **remove_labels = NULL;
	size_t remove_labels_size = 0;
	int rc = 0;

	(void) issue;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: expected label operations\n");
		return GCLI_EX_USAGE;
	}

	parse_labels_options(argc, argv, &add_labels, &add_labels_size,
	                     &remove_labels, &remove_labels_size);

	/* actually go about deleting and adding the labels */
	if (add_labels_size) {
		rc = gcli_issue_add_labels(g_clictx, issue_path, add_labels,
		                           add_labels_size);

		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to add labels: %s\n",
			        gcli_get_error(g_clictx));

			rc = GCLI_EX_DATAERR;
			goto bail;
		}
	}

	if (remove_labels_size) {
		rc = gcli_issue_remove_labels(g_clictx, issue_path,
		                              remove_labels, remove_labels_size);

		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to remove labels: %s\n",
			        gcli_get_error(g_clictx));

			rc = GCLI_EX_DATAERR;
			goto bail;
		}
	}

bail:
	free(add_labels);
	free(remove_labels);

	return rc;
}

static int
action_all(struct gcli_path const *const path, struct gcli_issue const *issue,
           int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_issue_print_summary(issue);

	puts("\nORIGINAL POST\n");
	gcli_issue_print_op(issue);

	return GCLI_EX_OK;
}

static int
action_comments(struct gcli_path const *const path,
                struct gcli_issue const *issue,
                int *argc, char **argv[])
{
	(void) issue;
	(void) argc;
	(void) argv;

	if (gcli_issue_comments(path) < 0) {
		fprintf(stderr, "gcli: error: failed to fetch issue comments: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_op(struct gcli_path const *const path,
          struct gcli_issue const *issue,
          int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_issue_print_op(issue);

	return GCLI_EX_OK;
}

static int
action_status(struct gcli_path const *const path,
              struct gcli_issue const *issue,
              int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_issue_print_summary(issue);

	return GCLI_EX_OK;
}

static int
action_close(struct gcli_path const *const path,
             struct gcli_issue const *issue,
             int *argc, char **argv[])
{
	(void) issue;
	(void) argc;
	(void) argv;

	if (gcli_issue_close(g_clictx, path) < 0) {
		fprintf(stderr, "gcli: error: failed to close issue: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_reopen(struct gcli_path const *const path,
              struct gcli_issue const *issue,
              int *argc, char **argv[])
{
	(void) issue;
	(void) argc;
	(void) argv;

	if (gcli_issue_reopen(g_clictx, path) < 0) {
		fprintf(stderr, "gcli: error: failed to reopen issue: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_assign(struct gcli_path const *const path,
              struct gcli_issue const *issue,
              int *argc, char **argv[])
{
	char const *assignee;

	(void) issue;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing assignee\n");
		return GCLI_EX_USAGE;
	}

	*argc -= 1;
	*argv += 1;
	assignee = (*argv)[0];

	if (gcli_issue_assign(g_clictx, path, assignee) < 0) {
		fprintf(stderr, "gcli: error: failed to assign issue: %s\n",
		     gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_milestone(struct gcli_path const *const path,
                 struct gcli_issue const *const issue,
                 int *argc, char ***argv)
{
	char const *milestone_str;
	char *endptr;
	int milestone, rc;

	(void) issue;

	/* Set the milestone for the issue
	 *
	 * Check that the user provided a milestone id */
	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing milestone id\n");
		return GCLI_EX_USAGE;
	}

	/* Fetch the milestone from the argument vector */
	*argc -= 1;
	*argv += 1;
	milestone_str = (*argv)[0];

	/* Check if the milestone_str is -d indicating that we should
	 * clear the milestone */
	if (strcmp(milestone_str, "-d") == 0) {
		rc = gcli_issue_clear_milestone(g_clictx, path);
		if (rc < 0) {
			fprintf(stderr,
			        "gcli: error: could not clear milestone of issue: %s\n",
			        gcli_get_error(g_clictx));

			return GCLI_EX_DATAERR;
		}

		return GCLI_EX_OK;
	}

	/* It is a milestone ID. Parse it. */
	milestone = strtoul(milestone_str, &endptr, 10);

	/* Check successful for parse */
	if (endptr != milestone_str + strlen(milestone_str)) {
		fprintf(stderr, "gcli: error: could not parse milestone id\n");
		return GCLI_EX_USAGE;
	}

	/* Pass it to the dispatch */
	if (gcli_issue_set_milestone(g_clictx, path, milestone) < 0) {
		fprintf(stderr, "gcli: error: could not assign milestone: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_title(struct gcli_path const *const path,
             struct gcli_issue const *const issue,
             int *argc, char **argv[])
{
	char const *new_title = NULL;
	int rc;

	(void) issue;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing new title\n");
		return GCLI_EX_USAGE;
	}

	*argc -= 1;
	*argv += 1;
	new_title = (*argv)[0];

	rc = gcli_issue_set_title(g_clictx, path, new_title);

	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to set new issue title: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static void
gcli_print_attachments(struct gcli_attachment_list const *const list)
{
	gcli_tbl tbl;
	struct gcli_tblcoldef columns[] = {
		{ .name = "ID",       .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "AUTHOR",   .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD     },
		{ .name = "CREATED",  .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0                    },
		{ .name = "CONTENT",  .type = GCLI_TBLCOLTYPE_STRING, .flags = 0                    },
		{ .name = "OBSOLETE", .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0                    },
		{ .name = "FILENAME", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0                    },
	};

	tbl = gcli_tbl_begin(columns, ARRAY_SIZE(columns));

	for (size_t i = 0; i < list->attachments_size; ++i) {
		struct gcli_attachment const *const it = &list->attachments[i];
		gcli_tbl_add_row(tbl, it->id, it->author, it->created_at,
		                 it->content_type, it->is_obsolete, it->file_name);
	}

	gcli_tbl_end(tbl);
}

static int
action_attachments(struct gcli_path const *const path,
                   struct gcli_issue const *const issue,
                   int *argc, char **argv[])
{
	struct gcli_attachment_list list = {0};

	(void) issue;
	(void) argc;
	(void) argv;

	int rc = gcli_issue_get_attachments(g_clictx, path, &list);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to fetch attachments: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	gcli_print_attachments(&list);
	gcli_attachments_free(&list);

	return GCLI_EX_OK;
}

struct gcli_cmd_actions issue_actions = {
	.fetch_item = (gcli_cmd_action_fetcher)gcli_get_issue,
	.free_item = (gcli_cmd_action_freeer)gcli_issue_free,
	.item_size = sizeof(struct gcli_issue),


	.defs = {
		{ .name = "all",         .needs_item = true,  .handler = (gcli_cmd_action_handler)action_all,         },
		{ .name = "comments",    .needs_item = false, .handler = (gcli_cmd_action_handler)action_comments,    },
		{ .name = "notes",       .needs_item = false, .handler = (gcli_cmd_action_handler)action_comments,    },
		{ .name = "op",          .needs_item = true,  .handler = (gcli_cmd_action_handler)action_op,          },
		{ .name = "status",      .needs_item = true,  .handler = (gcli_cmd_action_handler)action_status,      },
		{ .name = "close",       .needs_item = false, .handler = (gcli_cmd_action_handler)action_close,       },
		{ .name = "reopen",      .needs_item = false, .handler = (gcli_cmd_action_handler)action_reopen,      },
		{ .name = "assign",      .needs_item = false, .handler = (gcli_cmd_action_handler)action_assign,      },
		{ .name = "labels",      .needs_item = false, .handler = (gcli_cmd_action_handler)action_labels,      },
		{ .name = "milestone",   .needs_item = false, .handler = (gcli_cmd_action_handler)action_milestone,   },
		{ .name = "title",       .needs_item = false, .handler = (gcli_cmd_action_handler)action_title,       },
		{ .name = "attachments", .needs_item = false, .handler = (gcli_cmd_action_handler)action_attachments, },
	},
};

static int
handle_issues_actions(int argc, char *argv[],
                      struct gcli_path const *const path)
{
	int rc = 0;

	/* Check if the user missed out on supplying actions */
	if (argc == 0) {
		fprintf(stderr, "gcli: error: no actions supplied\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = gcli_cmd_actions_handle(&issue_actions, path, &argc, &argv);
	if (rc == GCLI_EX_USAGE) {
		usage();
	}

	return !!rc;
}
