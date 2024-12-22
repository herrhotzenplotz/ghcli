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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/config.h>
#include <gcli/cmd/forks.h>
#include <gcli/cmd/gitconfig.h>
#include <gcli/cmd/table.h>

#include <gcli/forks.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli forks create [-o owner -r repo] [-i target] [-y]\n");
	fprintf(stderr, "       gcli forks [-o owner -r repo] [-n number] [-s] [-y] [delete]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -i target       Name of org or user to create the fork in\n");
	fprintf(stderr, "  -n number       Number of forks to fetch (-1 = everything)\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_print_forks(enum gcli_output_flags const flags,
                 struct gcli_fork_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "OWNER",    .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "FORKS",    .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->forks_size == 0) {
		puts("No forks");
		return;
	}

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->forks_size)
		n = list->forks_size;
	else
		n = max;

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not initialize table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->forks[n-i-1].owner,
			                 list->forks[n-i-1].date,
			                 list->forks[n-i-1].forks,
			                 list->forks[n-i-1].full_name);
		}
	} else {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->forks[i].owner,
			                 list->forks[i].date,
			                 list->forks[i].forks,
			                 list->forks[i].full_name);
		}
	}

	gcli_tbl_end(table);
}

static int
subcommand_forks_create(int argc, char *argv[])
{
	bool always_yes = false;
	char *in = NULL;
	int ch;
	struct gcli_path repo_path = {0};

	struct option const options[] = {
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "into",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "yo:r:i:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			repo_path.data.as_default.owner = optarg;
			break;
		case 'r':
			repo_path.data.as_default.repo = optarg;
			break;
		case 'i':
			in = optarg;
			break;
		case 'y':
			always_yes = true;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&repo_path);

	if (gcli_fork_create(g_clictx, &repo_path, in) < 0)
		errx(1, "gcli: error: failed to fork repository: %s", gcli_get_error(g_clictx));

	if (!always_yes) {
		if (!sn_yesno("Do you want to add a remote for the fork?"))
			return EXIT_SUCCESS;
	}

	if (!in) {
		if ((in = gcli_config_get_account_name(g_clictx)) == NULL) {
			errx(1, "gcli: error: could not fetch account: %s",
			     gcli_get_error(g_clictx));
		}
	}

	gcli_gitconfig_add_fork_remote(in, repo_path.data.as_default.repo);

	return EXIT_SUCCESS;
}

int
subcommand_forks(int argc, char *argv[])
{
	struct gcli_fork_list forks = {0};
	struct gcli_path repo_path = {0};
	int ch = 0;
	int count = 30;
	bool always_yes = false;
	enum gcli_output_flags flags = 0;

	/* detect whether we wanna create a fork */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_forks_create(argc, argv);
	}

	struct option const options[] = {
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "n:o:r:ys", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			repo_path.data.as_default.owner = optarg;
			break;
		case 'r':
			repo_path.data.as_default.repo = optarg;
			break;
		case 'y':
			always_yes = true;
			break;
		case 'n': {
			char *endptr = NULL;
			count = strtol(optarg, &endptr, 10);

			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: unable to parse forks count argument");

			if (count == 0)
				errx(1, "gcli: error: forks count must not be zero");
		} break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&repo_path);

	if (argc == 0) {
		if (gcli_get_forks(g_clictx, &repo_path, count, &forks) < 0)
			errx(1, "gcli: error: could not get forks: %s", gcli_get_error(g_clictx));

		gcli_print_forks(flags, &forks, count);
		gcli_forks_free(&forks);

		return EXIT_SUCCESS;
	}

	for (size_t i = 0; i < (size_t)argc; ++i) {
		char const *action = argv[i];

		if (strcmp(action, "delete") == 0) {
			delete_repo(always_yes, &repo_path);
		} else {
			fprintf(stderr, "gcli: error: forks: unknown action '%s'\n", action);
		}
	}

	return EXIT_SUCCESS;
}
