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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <gcli/cmd/cmd.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pulls review [-o owner -r repo] -i id\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "   -o owner         Operate on a repository by the given owner\n");
	fprintf(stderr, "   -r repo          Operate on the given repository\n");
	fprintf(stderr, "   -i id            Operate on the given PR id\n");
	fprintf(stderr, "\n");
	version();
}

int
subcommand_pull_review(int argc, char *argv[])
{
	int ch;
	char const *owner = NULL, *repo = NULL;
	gcli_id pr_id;
	bool have_id = false;

	struct option options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o', },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r', },
		{ .name = "id",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'i', },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "+o:r:i:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			owner = optarg;
		} break;
		case 'r': {
			repo = optarg;
		} break;
		case 'i': {
			char *endptr;
			pr_id = strtoul(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "error: bad PR id %s\n", optarg);

			have_id = true;
		} break;
		default: {
			usage();
			return EXIT_FAILURE;
		} break;
		}
	}

	argc -= optind;
	argv += optind;

	if (!have_id) {
		fprintf(stderr, "error: missing PR id. use -i <id>.\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);

	(void) pr_id;
	fprintf(stderr, "error: not yet implemented\n");
	return EXIT_FAILURE;
}
