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
#include <gcli/cmd/repos.h>
#include <gcli/cmd/table.h>

#include <gcli/repos.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli repos create -r repo [-d description] [-p]\n");
	fprintf(stderr, "       gcli repos [-o owner -r repo] [-n number] [-s]\n");
	fprintf(stderr, "       gcli repos [-o owner -r repo] actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner                The repository owner\n");
	fprintf(stderr, "  -r repo                 The repository name\n");
	fprintf(stderr, "  -n number               Number of repos to fetch (-1 = everything)\n");
	fprintf(stderr, "  -p                      Make the repo private\n");
	fprintf(stderr, "  -s                      Print (sort) in reverse order\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  delete [-y]             Delete this repository:\n");
	fprintf(stderr, "                            -y    Do not ask for confirmation\n");
	fprintf(stderr, "  set-visibility <level>  Mark the reposity as public or private. Level may be one of:\n");
	fprintf(stderr, "                            - public\n");
	fprintf(stderr, "                            - private\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_print_repos(enum gcli_output_flags const flags,
                 struct gcli_repo_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "FORK",     .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0 },
		{ .name = "VISBLTY",  .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->repos_size == 0) {
		puts("No repos");
		return;
	}

	/* Determine number of repos to print */
	if (max < 0 || (size_t)(max) > list->repos_size)
		n = list->repos_size;
	else
		n = max;

	/* init table */
	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	/* put data into table */
	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->repos[n-i-1].is_fork,
			                 list->repos[n-i-1].visibility,
			                 list->repos[n-i-1].date,
			                 list->repos[n-i-1].full_name);
	} else {
		for (size_t i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->repos[i].is_fork,
			                 list->repos[i].visibility,
			                 list->repos[i].date,
			                 list->repos[i].full_name);
	}

	/* print it */
	gcli_tbl_end(table);
}

void
gcli_repo_print(struct gcli_repo const *it)
{
	gcli_dict dict;

	dict = gcli_dict_begin();
	gcli_dict_add(dict,           "ID",         0, 0, "%"PRIid, it->id);
	gcli_dict_add(dict,           "FULL NAME",  0, 0, "%s", it->full_name);
	gcli_dict_add(dict,           "NAME",       0, 0, "%s", it->name);
	gcli_dict_add(dict,           "OWNER",      0, 0, "%s", it->owner);
	gcli_dict_add_timestamp(dict, "DATE",       0, 0, it->date);
	gcli_dict_add(dict,           "VISIBILITY", 0, 0, "%s", it->visibility);
	gcli_dict_add(dict,           "IS FORK",    0, 0, "%s", sn_bool_yesno(it->is_fork));

	gcli_dict_end(dict);
}

static int
subcommand_repos_create(int argc, char *argv[])
{
	int ch;
	struct gcli_repo_create_options create_options = {0};
	struct gcli_repo repo = {0};

	const struct option options[] = {
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "private",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{ .name    = "description",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'd' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "r:d:p", options, NULL)) != -1) {
		switch (ch) {
		case 'r':
			create_options.name = optarg;
			break;
		case 'd':
			create_options.description = optarg;
			break;
		case 'p':
			create_options.private = true;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (!create_options.name) {
		fprintf(stderr,
		        "gcli: name cannot be empty. please set a repository "
		        "name with -r/--name\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_repo_create(g_clictx, &create_options, &repo) < 0) {
		errx(1, "gcli: error: failed to create repository: %s",
		     gcli_get_error(g_clictx));
	}

	gcli_repo_print(&repo);
	gcli_repo_free(&repo);

	return EXIT_SUCCESS;
}

static int
action_delete(char const *const owner, char const *const repo, int *argc,
              char ***argv)
{
	int ch;
	bool always_yes = false;
	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(*argc, *argv, "+y", options, NULL)) != -1) {
		switch (ch) {
		case 'y':
			always_yes = true;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	*argc -= optind;
	*argv += optind;

	delete_repo(always_yes, owner, repo);

	return 0;
}

static gcli_repo_visibility
parse_visibility(char const *str)
{
	if (strcmp(str, "public") == 0) {
		return GCLI_REPO_VISIBILITY_PUBLIC;
	} else if (strcmp(str, "private") == 0) {
		return GCLI_REPO_VISIBILITY_PRIVATE;
	} else {
		fprintf(stderr, "gcli: error: bad visibility level »%s«\n", str);
		return 1;
	}
}

/* Change the visibility level of a repository (e.g. public, private
 * etc) */
static int
action_set_visibility(char const *const owner, char const *const repo,
                      int *argc , char ***argv)
{
	char const *visblty_str;
	gcli_repo_visibility visblty;
	int rc;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing visibility level\n");
		return 1;
	}

	visblty_str = (*argv)[1];
	*argv += 2;
	*argc -= 2;

	visblty = parse_visibility(visblty_str);

	if ((rc = gcli_repo_set_visibility(g_clictx, owner, repo, visblty)) < 0) {
		fprintf(stderr, "gcli: error: failed to set visibility: %s\n",
		        gcli_get_error(g_clictx));
		return 1;
	}

	return 0;
}

static struct action {
	char const *const name;
	int (*fn)(char const *const owner, char const *const repo, int *argc,
	          char ***argv);
} const actions[] = {
	{ .name = "delete",         .fn = action_delete },
	{ .name = "set-visibility", .fn = action_set_visibility },
};

static size_t const actions_size = ARRAY_SIZE(actions);

static struct action const *
find_action(char const *const name)
{
	for (size_t i = 0; i < actions_size; ++i) {
		if (strcmp(name, actions[i].name) == 0)
			return &actions[i];
	}

	return NULL;
}

int
subcommand_repos(int argc, char *argv[])
{
	int ch, n = 30;
	char const *owner = NULL;
	char const *repo = NULL;
	struct gcli_repo_list repos = {0};
	enum gcli_output_flags flags = 0;

	/* detect whether we wanna create a repo */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_repos_create(argc, argv);
	}

	struct option const options[] = {
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "+n:o:r:s", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case 'n': {
			char *endptr = NULL;
			n = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse repo count");

			if (n == 0)
				errx(1, "gcli: error: number of repos must not be zero");
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

	/* List repos of the owner */
	if (argc == 0) {
		int rc = 0;

		if (repo) {
			fprintf(stderr, "gcli: error: no actions specified\n");
			usage();
			return EXIT_FAILURE;
		}

		if (!owner)
			owner = gcli_config_get_account_name(g_clictx);

		/* whenever there is no default account we would be passing NULL to
		 * gcli_get_repos. This is bad since that causes segfaults down the
		 * line. (https://github.com/herrhotzenplotz/gcli/issues/118) */
		if (!owner) {
			fprintf(stderr, "gcli: error: no account specified or no default"
			        " account configured. use -o to provide an explicit"
			        " account name.\n");
			return EXIT_FAILURE;
		}

		rc = gcli_get_repos(g_clictx, owner, n, &repos);
		if (rc < 0) {
			errx(1, "gcli: error: failed to fetch repos: %s",
			     gcli_get_error(g_clictx));
		}

		gcli_print_repos(flags, &repos, n);
		gcli_repos_free(&repos);
	} else {
		check_owner_and_repo(&owner, &repo);

		while (argc) {
			struct action const *action = find_action(argv[0]);
			int rc = 0;

			if (!action) {
				fprintf(stderr, "gcli: error: unrecognised action »%s«\n", argv[0]);
				return EXIT_FAILURE;
			}

			rc = action->fn(owner, repo, &argc, &argv);
			if (rc)
				return rc;
		}
	}

	return EXIT_SUCCESS;
}
