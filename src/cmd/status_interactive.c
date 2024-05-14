/*
 * Copyright 2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/comment.h>
#include <gcli/cmd/interactive.h>
#include <gcli/cmd/issues.h>
#include <gcli/cmd/status_interactive.h>
#include <gcli/cmd/table.h>

#include <gcli/status.h>

static void
print_notification_table(struct gcli_notification_list const *list)
{
	gcli_tbl *table;
	struct gcli_tblcoldef const columns[] = {
		{ .name = "NUMBER", .type = GCLI_TBLCOLTYPE_LONG,   .flags = 0 },
		{ .name = "REPO",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TYPE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REASON", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	table = gcli_tbl_begin(columns, ARRAY_SIZE(columns));

	for (size_t i = 0; i < list->notifications_size; ++i) {
		struct gcli_notification const *n = &list->notifications[i];
		gcli_tbl_add_row(table, (long)i + 1, n->repository,
		                 gcli_notification_target_type_str(n->type),
		                 n->reason);
	}

	gcli_tbl_end(table);
}

static void
refresh_notifications(struct gcli_notification_list *list)
{
	int rc = 0;

	gcli_free_notifications(list);

	rc = gcli_get_notifications(g_clictx, -1, list);
	if (rc < 0)
		errx(1, "gcli: failed to fetch notifications: %s", gcli_get_error(g_clictx));
}

static void
handle_issue_notification(struct gcli_notification const *const notif)
{
	char *user_input = NULL;
	int rc = 0;
	struct gcli_issue issue = {0};

	rc = gcli_notification_get_issue(g_clictx, notif, &issue);
	if (rc < 0)
		errx(1, "gcli: failed to fetch issue: %s", gcli_get_error(g_clictx));

	for (;;) {
		user_input = gcli_cmd_prompt( "[%s] What? (status, discussion, quit)", NULL, notif->repository);

		if (strcmp(user_input, "quit") == 0 ||
		    strcmp(user_input, "q") == 0) {
			break;

		} else if (strcmp(user_input, "status") == 0 ||
		           strcmp(user_input, "s") == 0) {
			gcli_issue_print_summary(&issue);
		} else if (strcmp(user_input, "discussion") == 0 ||
		           strcmp(user_input, "d") == 0) {
			struct gcli_comment_list comments = {0};

			rc = gcli_notification_get_comments(g_clictx, notif, &comments);
			if (rc < 0)
				errx(1, "gcli: failed to fetch comments: %s", gcli_get_error(g_clictx));

			gcli_print_comment_list(&comments);
			gcli_comments_free(&comments);
		}

		free(user_input);
		user_input = NULL;
	}

	gcli_issue_free(&issue);
	free(user_input);
	user_input = NULL;
}

typedef void (*notification_handler)(struct gcli_notification const *);
static notification_handler
notification_handlers[MAX_GCLI_NOTIFICATION_TARGET] = {
	[GCLI_NOTIFICATION_TARGET_ISSUE] = handle_issue_notification,
};

static void
status_interactive_notification(struct gcli_notification const *const notif)
{
	if (notif->type >= MAX_GCLI_NOTIFICATION_TARGET) {
		fprintf(stderr, "gcli: error: bad notification type\n");
		return;
	}

	if (notification_handlers[notif->type] == NULL) {
		fprintf(stderr, "gcli: error: notification type '%s' not supported\n",
		        gcli_notification_target_type_str(notif->type));
		return;
	}

	notification_handlers[notif->type](notif);
}

int
gcli_status_interactive(void)
{
	struct gcli_notification_list list = {0};
	char *user_input = NULL;

	refresh_notifications(&list);
	print_notification_table(&list);

	for (;;) {
		user_input = gcli_cmd_prompt("Enter number, list or quit", NULL);

		if (strcmp(user_input, "q") == 0 ||
		    strcmp(user_input, "quit") == 0) {
			goto out;

		} else if (strcmp(user_input, "l") == 0 ||
		           strcmp(user_input, "list") == 0) {
			refresh_notifications(&list);
			print_notification_table(&list);

		} else {
			size_t number = 0;
			char *endptr = NULL;

			number = strtoul(user_input, &endptr, 10);
			if (endptr != user_input + strlen(user_input)) {
				fprintf(stderr, "gcli: bad notification number: %s\n",
				        user_input);

				goto next;
			}

			if (number == 0 || number > list.notifications_size) {
				fprintf(stderr, "gcli: unknown notification number\n");
				goto next;
			}

			status_interactive_notification(&list.notifications[number-1]);
		}

	next:
		free(user_input);
		user_input = NULL;
	}

out:
	free(user_input);
	user_input = NULL;

	gcli_free_notifications(&list);

	return 0;
}
