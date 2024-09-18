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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/config.h>
#include <gcli/cmd/table.h>

#include <gcli/sshkeys.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli config ssh\n");
	fprintf(stderr, "       gcli config ssh add --title some-title --key path/to/key.pub\n");
	fprintf(stderr, "       gcli config ssh delete id\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_sshkeys_print_keys(struct gcli_sshkey_list const *list)
{
	gcli_tbl *tbl;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_ID,     .flags = 0 },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_TIME_T, .flags = 0 },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->keys_size == 0) {
		printf("No SSH keys\n");
		return;
	}

	tbl = gcli_tbl_begin(cols, ARRAY_SIZE(cols));

	for (size_t i = 0; i < list->keys_size; ++i) {
		gcli_tbl_add_row(tbl, list->keys[i].id, list->keys[i].created_at,
		                 list->keys[i].title);
	}

	gcli_tbl_end(tbl);
}

static int
list_sshkeys(void)
{
	struct gcli_sshkey_list list = {0};

	if (gcli_sshkeys_get_keys(g_clictx, &list) < 0) {
		fprintf(stderr, "gcli: error: could not get list of SSH keys\n");
		return EXIT_FAILURE;
	}

	gcli_sshkeys_print_keys(&list);
	gcli_sshkeys_free_keys(&list);

	return 0;
}

static int
add_sshkey(int argc, char *argv[])
{
	char *title = NULL, *keypath = NULL;
	int ch;

	struct option options[] = {
		{ .name = "title", .has_arg = required_argument, .flag = NULL, .val = 't' },
		{ .name = "key",   .has_arg = required_argument, .flag = NULL, .val = 'k' },
		{ 0 },
	};

	while ((ch = getopt_long(argc, argv, "+t:k:", options, NULL)) != -1) {
		switch (ch) {
		case 't': {
			title = optarg;
		} break;
		case 'k': {
			keypath = optarg;

			if (access(keypath, R_OK) < 0) {
				fprintf(stderr, "gcli: error: cannot access %s: %s\n",
				        keypath, strerror(errno));
				return EXIT_FAILURE;
			}
		} break;
		default: {
			usage();
			return EXIT_FAILURE;
		} break;
		}
	}

	if (title == NULL) {
		fprintf(stderr, "gcli: error: missing title\n");
		usage();
		return EXIT_FAILURE;
	}

	if (keypath == NULL) {
		fprintf(stderr, "gcli: error: missing public key path\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_sshkeys_add_key(g_clictx, title, keypath, NULL) < 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int
delete_sshkey(int argc, char *argv[])
{
	int id;
	char *endptr;

	/* skip 'delete' keyword */
	--argc; ++argv;

	if (argc != 1) {
		fprintf(stderr, "gcli: error: incorrect number of arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	/* parse the id */
	id = strtol(argv[0], &endptr, 10);

	if (endptr != argv[0] + strlen(argv[0])) {
		fprintf(stderr, "gcli: error: could not parse ID of SSH key to delete\n");
		return EXIT_FAILURE;
	}

	if (gcli_sshkeys_delete_key(g_clictx, id) < 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int
subcommand_ssh(int argc, char *argv[])
{
	char *cmdname;

	if (--argc == 0)
		return list_sshkeys();

	cmdname = *(++argv);

	if (strcmp(cmdname, "add") == 0)
		return add_sshkey(argc, argv);
	else if (strcmp(cmdname, "delete") == 0)
		return delete_sshkey(argc, argv);

	fprintf(stderr, "gcli: error: unrecognised subcommand »%s«.\n", cmdname);
	usage();
	return EXIT_FAILURE;
}

struct subcommand {
	char const *const name;
	int (*fn)(int argc, char *argv[]);
} subcommands[] = {
	{ .name = "ssh", .fn = subcommand_ssh },
};

int
subcommand_config(int argc, char *argv[])
{
	int ch;

	struct option options[] = {
		{0}
	};

	while ((ch = getopt_long(argc, argv, "+", options, NULL)) != -1) {
		switch (ch) {
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* Check if the user gave us at least one option for this
	 * subcommand */
	if (argc == 0) {
		fprintf(stderr, "gcli: error: missing subcommand for config\n");
		usage();
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < ARRAY_SIZE(subcommands); ++i) {
		if (strcmp(argv[0], subcommands[i].name) == 0)
			return subcommands[i].fn(argc, argv);
	}

	fprintf(stderr, "gcli: error: unrecognised config subcommand »%s«\n",
	        argv[0]);
	usage();

	return EXIT_FAILURE;
}
