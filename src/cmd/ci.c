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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd/ci.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/table.h>

#include <gcli/forges.h>
#include <gcli/github/checks.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli ci [-o owner -r repo] [-n number] ref\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -n number       Number of check runs to fetch (-1 = everything)\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
github_print_checks(struct github_check_list const *const list)
{
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "CONCLUSION", .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "COMPLETED",  .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->checks_size) {
		fprintf(stderr, "No checks\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	for (size_t i = 0; i < list->checks_size; ++i) {
		gcli_tbl_add_row(table, list->checks[i].id, list->checks[i].status,
		                 list->checks[i].conclusion, list->checks[i].started_at,
		                 list->checks[i].completed_at, list->checks[i].name);
	}

	gcli_tbl_end(table);
}

int
github_checks(struct gcli_path const *const path, char const *const ref,
              int const max)
{
	struct github_check_list list = {0};
	int rc = 0;

	rc = github_get_checks(g_clictx, path, ref, max, &list);
	if (rc < 0)
		return rc;

	github_print_checks(&list);
	github_free_checks(&list);

	return rc;
}

int
subcommand_ci(int argc, char *argv[])
{
	int ch = 0;
	struct gcli_path repo_path = {0};
	char const *ref = NULL;
	int count = -1;             /* fetch all checks by default */

	/* Parse options */
	struct option const options[] = {
		{.name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r'},
		{.name = "owner", .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "count", .has_arg = required_argument, .flag = NULL, .val = 'c'},
		{0}
	};

	while ((ch = getopt_long(argc, argv, "n:o:r:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			repo_path.data.as_default.owner = optarg;
			break;
		case 'r':
			repo_path.data.as_default.repo = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "ci: cannot parse argument to -n");
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* Check that we have exactly one left argument and print proper
	 * error messages */
	if (argc < 1) {
		fprintf(stderr, "gcli: error: missing ref\n");
		usage();
		return EXIT_FAILURE;
	}

	if (argc > 1) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Save the ref */
	ref = argv[0];

	check_path(&repo_path);

	/* Make sure we are actually talking about a github remote because
	 * we might be incorrectly inferring it */
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITHUB)
		errx(1, "gcli: error: The ci subcommand only works for GitHub. "
		     "Use gcli -t github ... to force a GitHub remote.");

	if (github_checks(&repo_path, ref, count) < 0)
		errx(1, "gcli: error: failed to get github checks: %s",
		     gcli_get_error(g_clictx));


	return EXIT_SUCCESS;
}
