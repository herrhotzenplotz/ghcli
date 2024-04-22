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
#include <gcli/cmd/interactive.h>
#include <gcli/cmd/status_interactive.h>
#include <gcli/cmd/table.h>

#include <gcli/status.h>

static void
print_notification_table(struct gcli_notification_list const *list)
{
	gcli_tbl *table;
	struct gcli_tblcoldef const columns[] = {
		{ .name = "ID",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REPO",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TYPE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REASON", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	table = gcli_tbl_begin(columns, ARRAY_SIZE(columns));

	for (size_t i = 0; i < list->notifications_size; ++i) {
		struct gcli_notification const *n = &list->notifications[i];
		gcli_tbl_add_row(table, n->id, n->repository, n->type, n->reason);
	}

	gcli_tbl_end(table);
}

int
gcli_status_interactive(void)
{
	struct gcli_notification_list notifications = {0};
	int rc = 0;
	char *user_input = NULL;

	rc = gcli_get_notifications(g_clictx, -1, &notifications);
	if (rc < 0)
		errx(1, "Failed to fetch notifications: %s", gcli_get_error(g_clictx));

	print_notification_table(&notifications);

	for (;;) {
		user_input = gcli_cmd_prompt("ID or quit", NULL);

		if (strcmp(user_input, "q") == 0 ||
		    strcmp(user_input, "quit") == 0) {
			goto out;
		}

		free(user_input);
		user_input = NULL;
	}

out:
	free(user_input);
	user_input = NULL;

	return 0;
}
