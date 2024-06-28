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
#include <gcli/cmd/table.h>

#include <gcli/curl.h>
#include <gcli/github/gists.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>
#include <unistd.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli gists [-u user] [-n number] [-s]\n");
	fprintf(stderr, "       gcli gists create [-d description] [-f file] name\n");
	fprintf(stderr, "       gcli gists delete [-y] id\n");
	fprintf(stderr, "       gcli gists get id name\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -d description  Description of the gist\n");
	fprintf(stderr, "  -f file         Path to file to upload (otherwise stdin)\n");
	fprintf(stderr, "  -l              Print a long list instead of a short table\n");
	fprintf(stderr, "  -n number       Number of gists to fetch\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -u user         User for whom to list gists\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static char const *
human_readable_size(size_t const s)
{
	if (s < 1024)
		return sn_asprintf("%zu B", s);

	if (s < 1024 * 1024)
		return sn_asprintf("%zu KiB", s / 1024);

	if (s < 1024 * 1024 * 1024)
		return sn_asprintf("%zu MiB", s / (1024 * 1024));

	return sn_asprintf("%zu GiB", s / (1024 * 1024 * 1024));
}

static inline char const *
language_fmt(char const *it)
{
	if (it)
		return it;
	else
		return "Unknown";
}

static void
print_gist_file(struct gcli_gist_file const *const file)
{
	printf("      • %-15.15s  %-8.8s  %-s\n",
	       language_fmt(file->language),
	       human_readable_size(file->size),
	       file->filename);
}

static void
print_gist(enum gcli_output_flags const flags, struct gcli_gist const *const gist)
{
	(void) flags;

	printf("   ID : %s%s%s\n"
	       "OWNER : %s%s%s\n"
	       "DESCR : %s\n"
	       " DATE : %s\n"
	       "  URL : %s\n"
	       " PULL : %s\n",
	       gcli_setcolour(GCLI_COLOR_YELLOW), gist->id, gcli_resetcolour(),
	       gcli_setbold(), gist->owner, gcli_resetbold(),
	       gist->description,
	       gist->date,
	       gist->url,
	       gist->git_pull_url);
	printf("FILES : %-15.15s  %-8.8s  %-s\n",
	       "LANGUAGE", "SIZE", "FILENAME");

	for (size_t i = 0; i < gist->files_size; ++i)
		print_gist_file(&gist->files[i]);

	printf("\n");
}

static void
gcli_print_gists_long(enum gcli_output_flags const flags,
                      struct gcli_gist_list const *const list, int const max)
{
	size_t n;

	if (max < 0 || (size_t)(max) > list->gists_size)
		n = list->gists_size;
	else
		n = max;

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i)
			print_gist(flags, &list->gists[n-i-1]);
	} else {
		for (size_t i = 0; i < n; ++i)
			print_gist(flags, &list->gists[i]);
	}
}

static void
gcli_print_gists_short(enum gcli_output_flags const flags,
                       struct gcli_gist_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",          .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_COLOUREXPL },
		{ .name = "OWNER",       .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "DATE",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FILES",       .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "DESCRIPTION", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (max < 0 || (size_t)(max) > list->gists_size)
		n = list->gists_size;
	else
		n = max;

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 GCLI_COLOR_YELLOW, list->gists[n-i-1].id,
			                 list->gists[n-i-1].owner,
			                 list->gists[n-i-1].date,
			                 (int)list->gists[n-i-1].files_size, /* For safety pass it as int */
			                 list->gists[n-i-1].description);
		}
	} else {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 GCLI_COLOR_YELLOW, list->gists[i].id,
			                 list->gists[i].owner,
			                 list->gists[i].date,
			                 (int)list->gists[i].files_size,
			                 list->gists[i].description);
		}
	}

	gcli_tbl_end(table);
}

void
gcli_print_gists(enum gcli_output_flags const flags,
                 struct gcli_gist_list const *const list, int const max)
{
	if (list->gists_size == 0) {
		puts("No Gists");
		return;
	}

	if (flags & OUTPUT_LONG)	/* if we are in long mode (no pun intended) */
		gcli_print_gists_long(flags, list, max);
	else                        /* real mode (bad joke, I know) */
		gcli_print_gists_short(flags, list, max);
}

static int
subcommand_gist_get(int argc, char *argv[])
{
	shift(&argc, &argv); /* Discard the *get* */

	char const *gist_id = shift(&argc, &argv);
	char const *file_name = shift(&argc, &argv);
	struct gcli_gist gist = {0};
	struct gcli_gist_file *file = NULL;

	if (gcli_get_gist(g_clictx, gist_id, &gist) < 0)
		errx(1, "gcli: error: failed to get gist: %s", gcli_get_error(g_clictx));

	for (size_t f = 0; f < gist.files_size; ++f) {
		if (strcmp(gist.files[f].filename, file_name) == 0) {
			file = &gist.files[f];
			break;
		}
	}

	if (!file)
		errx(1, "gcli: error: %s: no such file in gist with id %s",
		     file_name, gist_id);

	if (isatty(STDOUT_FILENO) && (file->size >= 4 * 1024 * 1024))
		errx(1, "gcli: error: File is bigger than 4 MiB, refusing to print to stdout.");

	if (gcli_curl(g_clictx, stdout, file->url, file->type) < 0)
		errx(1, "gcli: error: failed to fetch gist: %s", gcli_get_error(g_clictx));

	gcli_gist_free(&gist);

	return EXIT_SUCCESS;
}

static int
subcommand_gist_create(int argc, char *argv[])
{
	char const *file = NULL;
	int ch;
	struct gcli_new_gist opts = {0};

	struct option const options[] = {
		{ .name    = "file",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "description",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'd' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "f:d:", options, NULL)) != -1) {
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'd':
			opts.gist_description = optarg;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		fprintf(stderr, "gcli: error: missing file name for gist\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.file_name = shift(&argc, &argv);

	if (file) {
		if ((opts.file = fopen(file, "r")) == NULL)
			err(1, "gcli: error: cannot open file");
	} else {
		opts.file = stdin;
	}

	if (!opts.gist_description)
		opts.gist_description = "gcli paste";

	if (gcli_create_gist(g_clictx, opts) < 0)
		errx(1, "gcli: error: failed to create gist: %s", gcli_get_error(g_clictx));

	return EXIT_SUCCESS;
}

static int
subcommand_gist_delete(int argc, char *argv[])
{
	bool always_yes = false;
	char const *gist_id = NULL;
	int ch;

	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "y", options, NULL)) != -1) {
		switch (ch) {
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

	gist_id = shift(&argc, &argv);

	if (!always_yes && !sn_yesno("Are you sure you want to delete this gist?"))
		errx(1, "gcli: Aborted by user");

	gcli_delete_gist(g_clictx, gist_id);

	return EXIT_SUCCESS;
}

static struct {
	char const *name;
	int (*fn)(int, char **);
} gist_subcommands[] = {
	{ .name = "get",    .fn = subcommand_gist_get    },
	{ .name = "create", .fn = subcommand_gist_create },
	{ .name = "delete", .fn = subcommand_gist_delete },
};

int
subcommand_gists(int argc, char *argv[])
{
	char const *user = NULL;
	enum gcli_output_flags flags = 0;
	int ch;
	int count = 30;
	struct gcli_gist_list gists = {0};

	/* Make sure we are looking at a GitHub forge */
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITHUB) {
		errx(1, "gcli: error: The gists subcommand only works for GitHub "
		     "forges. Please use either -a or -t to force using a "
		     "GitHub account.");
	}

	for (size_t i = 0; i < ARRAY_SIZE(gist_subcommands); ++i) {
		if (argc > 1 && strcmp(argv[1], gist_subcommands[i].name) == 0) {
			argc -= 1;
			argv += 1;
			return gist_subcommands[i].fn(argc, argv);
		}
	}

	struct option const options[] = {
		{ .name    = "user",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'u' },
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{ .name    = "long",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'l' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "lsn:u:", options, NULL)) != -1) {
		switch (ch) {
		case 'u':
			user = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count        = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gists: cannot parse gists count");
		} break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case 'l':
			flags |= OUTPUT_LONG;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (gcli_get_gists(g_clictx, user, count, &gists) < 0)
		errx(1, "gcli: error: failed to get gists: %s", gcli_get_error(g_clictx));

	gcli_print_gists(flags, &gists, count);
	gcli_gists_free(&gists);

	return EXIT_SUCCESS;
}
