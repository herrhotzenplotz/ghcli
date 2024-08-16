/*
 * Copyright 2022, 2023, 2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/cmd/status.h>
#include <gcli/cmd/status_interactive.h>

#include <gcli/status.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli status -m id\n");
	fprintf(stderr, "       gcli status -l [-n number]\n");
	fprintf(stderr, "       gcli status\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -l          Print list of todo items and exit\n");
	fprintf(stderr, "  -n number   Number of messages to fetch\n");
	fprintf(stderr, "  -m id       Mark the given message as read\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

int
gcli_status_list(int const count)
{
	struct gcli_notification_list list = {0};
	int rc = 0;

	rc = gcli_get_notifications(g_clictx, count, &list);
	if (rc < 0)
		return rc;

	gcli_print_notifications(&list);
	gcli_free_notifications(&list);

	return rc;
}

void
gcli_print_notifications(struct gcli_notification_list const *const list)
{
	for (size_t i = 0; i < list->notifications_size; ++i) {
		printf("%s - %s - %s",
		       list->notifications[i].id,
		       list->notifications[i].repository,
		       gcli_notification_target_type_str(list->notifications[i].type));

		/* XXX what about target_id being zero? does any forge generate
		 *     zero for valid IDs? */
		if (list->notifications[i].target.id)
			printf(" %"PRIid, list->notifications[i].target.id);

		printf(" - %s", list->notifications[i].date);

		if (list->notifications[i].reason)
			printf(" - %s", list->notifications[i].reason);

		printf("\n");

		gcli_pretty_print(list->notifications[i].title, 4, 80, stdout);
		putchar('\n');
	}
}

int
subcommand_status(int argc, char *argv[])
{
	int count = 30, ch = 0, mark = 0, list = 0;
	char *endptr = NULL;

	const struct option options[] = {
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "mark",
		  .has_arg = no_argument,
		  .flag    = &mark,
		  .val     = 1 },
		{ .name    = "list",
		  .has_arg = no_argument,
		  .flag    = &list,
		  .val     = 1 },
		{0}
	};

	while ((ch = getopt_long(argc, argv, "n:ml", options, NULL)) != -1) {
		switch (ch) {
		case 'n': {
			count = strtol(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				err(1, "gcli: error: cannot parse parameter to -n");
		} break;
		case 'm': {
			mark = 1;
		} break;
		case 'l': {
			list = 1;
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (list) {
		if (mark) {
			fprintf(stderr, "gcli: cannot use -m and -l together\n");
			usage();
			return EXIT_FAILURE;
		}

		gcli_status_list(count);

	} else if (mark) {
		if (count != 30)
			warnx("gcli: ignoring -n/--count argument");

		if (argc > 1) {
			fprintf(stderr, "gcli: error: too many arguments for marking notifications\n");
			usage();
			return EXIT_FAILURE;
		}

		if (argc < 1) {
			fprintf(stderr, "gcli: error: missing notification id to mark as read\n");
			usage();
			return EXIT_FAILURE;
		}

		if (gcli_notification_mark_as_read(g_clictx, argv[0]) < 0)
			errx(1, "gcli: error: failed to mark the notification as read: %s",
			     gcli_get_error(g_clictx));
	} else {
		return gcli_status_interactive();
	}

	return EXIT_SUCCESS;
}
