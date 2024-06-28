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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>

#include <gcli/ctx.h>
#include <gcli/curl.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli api [-a] <path>\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -a            Fetch all pages of data\n");
	fprintf(stderr, "  path          Path to put after the API base URL\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static void
fetch_all(char *_url)
{
	char *url = NULL, *next_url = NULL;

	url = _url;

	do {
		struct gcli_fetch_buffer buffer = {0};

		if (gcli_fetch(g_clictx, url, &next_url, &buffer) < 0)
			errx(1, "gcli: error: failed to fetch data: %s",
			     gcli_get_error(g_clictx));

		fwrite(buffer.data, buffer.length, 1, stdout);

		gcli_fetch_buffer_free(&buffer);

		if (url != _url)
			free(url);

	} while ((url = next_url));
}

int
subcommand_api(int argc, char *argv[])
{
	char *url = NULL, *path = NULL;
	int ch, do_all = 0;

	struct option options[] = {
		{ .name = "all", .has_arg = no_argument, .flag = NULL, .val = 'a' },
		{0}
	};

	while ((ch = getopt_long(argc, argv, "+a", options, NULL)) != -1) {
		switch (ch) {
		case 'a':
			do_all = 1;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 1) {
		path = shift(&argc, &argv);
	} else {
		if (!argc)
			errx(1, "gcli: error: missing path");
		else
			errx(1, "gcli: error: too many arguments");
	}

	if (path[0] == '/')
		url = sn_asprintf("%s%s", gcli_get_apibase(g_clictx), path);
	else
		url = sn_asprintf("%s/%s", gcli_get_apibase(g_clictx), path);

	if (do_all)
		fetch_all(url);
	else if (gcli_curl(g_clictx, stdout, url, "application/json") < 0)
		errx(1, "gcli: error: failed to fetch data: %s",
		     gcli_get_error(g_clictx));

	free(url);

	return EXIT_SUCCESS;
}
