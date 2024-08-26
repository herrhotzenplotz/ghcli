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
#include <gcli/cmd/releases.h>
#include <gcli/cmd/table.h>
#include <gcli/cmd/editor.h>

#include <gcli/releases.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli releases create [-o owner -r repo] [-n name] "
	        "[-y] [-d] [-p] [-a asset]\n");
	fprintf(stderr, "                            [-c commitish] [-t tag]\n");
	fprintf(stderr, "       gcli releases delete [-o owner -r repo] [-y] id\n");
	fprintf(stderr, "       gcli releases [-o owner -r repo] [-n number] [-s] [-l]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -a asset        Path to file to upload as release asset\n");
	fprintf(stderr, "  -c committish   A ref/commit/branch that the release is created from\n");
	fprintf(stderr, "  -d              Mark as a release draft\n");
	fprintf(stderr, "  -l              Print a long list instead of a short table\n");
	fprintf(stderr, "  -n name         Name of the created release\n");
	fprintf(stderr, "  -n number       Number of releases to fetch (-1 = everything)\n");
	fprintf(stderr, "  -p              Mark as a prerelease\n");
	fprintf(stderr, "  -t tag          Name for new tag\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static void
gcli_print_release(enum gcli_output_flags const flags,
                   struct gcli_release const *const it)
{
	gcli_dict dict;

	(void) flags;

	dict = gcli_dict_begin();

	gcli_dict_add(dict,        "ID",         0, 0, "%s", it->id);
	gcli_dict_add(dict,        "NAME",       0, 0, "%s", it->name);
	gcli_dict_add(dict,        "AUTHOR",     0, 0, "%s", it->author);
	gcli_dict_add(dict,        "DATE",       0, 0, "%s", it->date);
	gcli_dict_add_string(dict, "DRAFT",      0, 0, sn_bool_yesno(it->draft));
	gcli_dict_add_string(dict, "PRERELEASE", 0, 0, sn_bool_yesno(it->prerelease));
	gcli_dict_add_string(dict, "ASSETS",     0, 0, "");

	/* asset urls */
	for (size_t i = 0; i < it->assets_size; ++i) {
		gcli_dict_add(dict, "", 0, 0, "• %s", it->assets[i].name);
		gcli_dict_add(dict, "", 0, 0, "  %s", it->assets[i].url);
	}

	gcli_dict_end(dict);

	/* body */
	if (it->body) {
		putchar('\n');
		gcli_pretty_print(it->body, 13, 80, stdout);
	}

	putchar('\n');
}

static void
gcli_releases_print_long(enum gcli_output_flags const flags,
                         struct gcli_release_list const *const list, int const max)
{
	int n;

	/* Determine how many items to print */
	if (max < 0 || (size_t)(max) > list->releases_size)
		n = list->releases_size;
	else
		n = max;

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i)
			gcli_print_release(flags, &list->releases[n-i-1]);
	} else {
		for (int i = 0; i < n; ++i)
			gcli_print_release(flags, &list->releases[i]);
	}
}

static void
gcli_releases_print_short(enum gcli_output_flags const flags,
                          struct gcli_release_list const *const list,
                          int const max)
{
	size_t n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DATE",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DRAFT",      .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0 },
		{ .name = "PRERELEASE", .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0 },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (max < 0 || (size_t)(max) > list->releases_size)
		n = list->releases_size;
	else
		n = max;

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->releases[n-i-1].id,
			                 list->releases[n-i-1].date,
			                 list->releases[n-i-1].draft,
			                 list->releases[n-i-1].prerelease,
			                 list->releases[n-i-1].name);
		}
	} else {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->releases[i].id,
			                 list->releases[i].date,
			                 list->releases[i].draft,
			                 list->releases[i].prerelease,
			                 list->releases[i].name);
		}
	}

	gcli_tbl_end(table);
}

void
gcli_releases_print(enum gcli_output_flags const flags,
                    struct gcli_release_list const *const list, int const max)
{
	if (list->releases_size == 0) {
		puts("No releases");
		return;
	}

	if (flags & OUTPUT_LONG)
		gcli_releases_print_long(flags, list, max);
	else
		gcli_releases_print_short(flags, list, max);
}

static void
releasemsg_init(struct gcli_ctx *ctx, FILE *f, void *_data)
{
	struct gcli_new_release const *info = _data;

	(void) ctx;

	fprintf(
		f,
		"! Enter your release notes above, save and exit.\n"
		"! All lines with a leading '!' are discarded and will not\n"
		"! appear in the final release note.\n"
		"!       IN : %s/%s\n"
		"! TAG NAME : %s\n"
		"!     NAME : %s\n",
		info->owner, info->repo, info->tag, info->name);
}

static char *
get_release_message(struct gcli_new_release const *info)
{
	return gcli_editor_get_user_message(g_clictx, releasemsg_init,
	                                    (void *)info);
}

static int
subcommand_releases_create(int argc, char *argv[])
{
	struct gcli_new_release release = {0};
	int ch;
	bool always_yes = false;

	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{ .name    = "draft",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'd' },
		{ .name    = "prerelease",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{ .name    = "name",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "tag",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 't' },
		{ .name    = "commitish",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'c' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "asset",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'a' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "ydpn:t:c:r:o:a:",
	                         options, NULL)) != -1) {
		switch (ch) {
		case 'd':
			release.draft = true;
			break;
		case 'p':
			release.prerelease = true;
			break;
		case 'n':
			release.name = optarg;
			break;
		case 't':
			release.tag = optarg;
			break;
		case 'c':
			release.commitish = optarg;
			break;
		case 'r':
			release.repo = optarg;
			break;
		case 'o':
			release.owner = optarg;
			break;
		case 'a': {
			struct gcli_release_asset_upload asset = {
				.path  = optarg,
				.name  = optarg,
				.label = "unused",
			};
			if (gcli_release_push_asset(g_clictx, &release, asset) < 0) {
				errx(1, "gcli: error: failed to add asset: %s",
				     gcli_get_error(g_clictx));
			}
		} break;
		case 'y': {
			always_yes = true;
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&release.owner, &release.repo);

	/* make sure we have a tag for the release */
	if (!release.tag) {
		fprintf(stderr, "gcli: error: releases create: missing tag name\n");
		usage();
		return EXIT_FAILURE;
	}

	release.body = get_release_message(&release);
	if (release.body == NULL)
		errx(1, "gcli: empty message. aborting.");

	if (!always_yes)
		if (!sn_yesno("Do you want to create this release?"))
			errx(1, "gcli: Aborted by user");

	if (gcli_create_release(g_clictx, &release) < 0) {
		errx(1, "gcli: error: failed to create release: %s",
		     gcli_get_error(g_clictx));
	}

	return EXIT_SUCCESS;
}

static int
subcommand_releases_delete(int argc, char *argv[])
{
	int         ch;
	char const *owner = NULL, *repo = NULL;
	bool        always_yes = false;

	struct option const options[] = {
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0}
	};

	while ((ch = getopt_long(argc, argv, "yo:r:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
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

	check_owner_and_repo(&owner, &repo);

	/* make sure the user supplied the release id */
	if (argc != 1) {
		fprintf(stderr, "gcli: error: releases delete: missing release id\n");
		usage();
		return EXIT_FAILURE;
	}

	if (!always_yes)
		if (!sn_yesno("Are you sure you want to delete this release?"))
			errx(1, "gcli: Aborted by user");

	if (gcli_delete_release(g_clictx, owner, repo, argv[0]) < 0) {
		errx(1, "gcli: error: failed to delete the release: %s",
		     gcli_get_error(g_clictx));
	}

	return EXIT_SUCCESS;
}

static struct {
	char const *name;
	int (*fn)(int, char **);
} releases_subcommands[] = {
	{ .name = "delete", .fn = subcommand_releases_delete },
	{ .name = "create", .fn = subcommand_releases_create },
};

int
subcommand_releases(int argc, char *argv[])
{
	int ch;
	int count = 30;
	char const *owner = NULL;
	char const *repo = NULL;
	struct gcli_release_list releases = {0};
	enum gcli_output_flags flags = 0;

	if (argc > 1) {
		for (size_t i = 0; i < ARRAY_SIZE(releases_subcommands); ++i) {
			if (strcmp(releases_subcommands[i].name, argv[1]) == 0)
				return releases_subcommands[i].fn(argc - 1, argv + 1);
		}
	}

	/* List releases if none of the subcommands matched */

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
		{ .name    = "long",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'l' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{0}
	};

	while ((ch = getopt_long(argc, argv, "sn:o:r:l", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count        = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse release count");

			if (count == 0)
				errx(1, "gcli: error: number of releases must not be zero");

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

	/* sanity check */
	if (argc > 0) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);

	if (gcli_get_releases(g_clictx, owner, repo, count, &releases) < 0) {
		errx(1, "gcli: error: could not get releases: %s",
		     gcli_get_error(g_clictx));
	}

	gcli_releases_print(flags, &releases, count);

	gcli_free_releases(&releases);

	return EXIT_SUCCESS;
}
