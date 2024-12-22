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

#include <gcli/labels.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli labels create [-o owner -r repo] -n name -c colour -d description\n");
	fprintf(stderr, "       gcli labels delete [-o owner -r repo] id\n");
	fprintf(stderr, "       gcli labels [-o owner -r repo] [-n number]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -n number       Number of labels to fetch (-1 = everything)\n");
	fprintf(stderr, "  -l name         Name of the new label\n");
	fprintf(stderr, "  -c colour       Six digit hex code of the label's colour\n");
	fprintf(stderr, "  -d description  A short description of the label\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_labels_print(struct gcli_label_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",          .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "",            .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_256COLOUR|GCLI_TBLCOL_TIGHT },
		{ .name = "NAME",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DESCRIPTION", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->labels_size)
		n = list->labels_size;
	else
		n = max;

	/* Fill table */
	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	for (size_t i = 0; i < n; ++i) {
		gcli_tbl_add_row(table,
		                 (long)list->labels[i].id, /* Cast is important here (#165) */
		                 list->labels[i].colour,
		                 gcli_config_have_colours(g_clictx) ? "  " : "",
		                 list->labels[i].name,
		                 list->labels[i].description);
	}

	gcli_tbl_end(table);
}

static int
subcommand_labels_delete(int argc, char *argv[])
{
	int ch, rc;
	struct gcli_path repo_path = {0};
	const struct option options[] = {
		{.name = "repo",  .has_arg = required_argument, .val = 'r'},
		{.name = "owner", .has_arg = required_argument, .val = 'o'},
		{0},
	};

	while ((ch = getopt_long(argc, argv, "o:r:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			repo_path.data.as_default.owner = optarg;
			break;
		case 'r':
			repo_path.data.as_default.repo = optarg;
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

	if (argc != 1) {
		fprintf(stderr, "gcli: error: missing label to delete\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = gcli_delete_label(g_clictx, &repo_path, argv[0]);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: couldn't delete label\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
subcommand_labels_create(int argc, char *argv[])
{
	int ch;
	struct gcli_label label = {0};
	struct gcli_label_list labels = { .labels = &label, .labels_size = 1 };
	struct gcli_path repo_path = {0};

	const struct option options[] = {
		{.name = "repo",        .has_arg = required_argument, .val = 'r'},
		{.name = "owner",       .has_arg = required_argument, .val = 'o'},
		{.name = "name",        .has_arg = required_argument, .val = 'n'},
		{.name = "colour",      .has_arg = required_argument, .val = 'c'},
		{.name = "description", .has_arg = required_argument, .val = 'd'},
		{0}
	};

	while ((ch = getopt_long(argc, argv, "n:o:r:d:c:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			repo_path.data.as_default.owner = optarg;
			break;
		case 'r':
			repo_path.data.as_default.repo = optarg;
			break;
		case 'c': {
			char *endptr = NULL;
			if (strlen(optarg) != 6)
				err(1, "gcli: error: colour must be a six-digit hexadecimal colour code");

			label.colour = strtol(optarg, &endptr, 16);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse colour");
		} break;
		case 'd': {
			label.description = optarg;
		} break;
		case 'n': {
			label.name = optarg;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&repo_path);

	if (!label.name) {
		fprintf(stderr, "gcli: error: missing name for label\n");
		usage();
		return EXIT_FAILURE;
	}

	if (!label.description) {
		fprintf(stderr, "gcli: error: missing description for label\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_create_label(g_clictx, &repo_path, &label) < 0) {
		errx(1, "gcli: error: failed to create label: %s",
		     gcli_get_error(g_clictx));
	}

	/* only if we are not quieted */
	if (!sn_quiet())
		gcli_labels_print(&labels, 1);

	gcli_free_label(&label);

	return EXIT_SUCCESS;
}

static struct {
	char const *name;
	int (*fn)(int, char **);
} labels_subcommands[] = {
	{ .name = "delete", .fn = subcommand_labels_delete },
	{ .name = "create", .fn = subcommand_labels_create },
};

int
subcommand_labels(int argc, char *argv[])
{
	int count = 30;
	int ch;
	struct gcli_path repo_path = {0};
	struct gcli_label_list labels = {0};

	const struct option options[] = {
		{.name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r'},
		{.name = "owner", .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "count", .has_arg = required_argument, .flag = NULL, .val = 'n'},
		{0}
	};

	if (argc > 1) {
		for (size_t i = 0; i < ARRAY_SIZE(labels_subcommands); ++i) {
			if (strcmp(labels_subcommands[i].name, argv[1]) == 0)
				return labels_subcommands[i].fn(argc - 1, argv + 1);
		}
	}

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
				errx(1, "gcli: error: cannot parse label count");

			if (count == 0)
				errx(1, "gcli: error: number of labels must not be zero");
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* sanity check: we must have parsed everything by now */
	if (argc > 0) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	check_path(&repo_path);

	if (gcli_get_labels(g_clictx, &repo_path, count, &labels) < 0)
		errx(1, "gcli: error: could not fetch list of labels: %s",
		     gcli_get_error(g_clictx));

	gcli_labels_print(&labels, count);

	gcli_free_labels(&labels);

	return EXIT_SUCCESS;
}
