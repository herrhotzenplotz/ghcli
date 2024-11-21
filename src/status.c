/*
 * Copyright 2022-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/status.h>
#include <gcli/forges.h>

int
gcli_get_notifications(struct gcli_ctx *ctx, int const max,
                       struct gcli_notification_list *const out)
{
	gcli_null_check_call(get_notifications, ctx, max, out);
}

void
gcli_free_notification(struct gcli_notification *const notification)
{
	free(notification->id);
	free(notification->title);
	free(notification->reason);
	free(notification->date);
	free(notification->repository);
	free(notification->target.url);
}

void
gcli_free_notifications(struct gcli_notification_list *list)
{
	for (size_t i = 0; i < list->notifications_size; ++i) {
		gcli_free_notification(&list->notifications[i]);
	}

	free(list->notifications);
	list->notifications = NULL;
	list->notifications_size = 0;
}

int
gcli_notification_mark_as_read(struct gcli_ctx *ctx, char const *id)
{
	gcli_null_check_call(notification_mark_as_read, ctx, id);
}

static char const *
notification_target_type_strings[MAX_GCLI_NOTIFICATION_TARGET] = {
	[GCLI_NOTIFICATION_TARGET_INVALID] = "Invalid",
	[GCLI_NOTIFICATION_TARGET_ISSUE] = "Issue",
	[GCLI_NOTIFICATION_TARGET_PULL_REQUEST] = "Pull Request",
	[GCLI_NOTIFICATION_TARGET_COMMIT] = "Commit",
	[GCLI_NOTIFICATION_TARGET_EPIC] = "Epic",
	[GCLI_NOTIFICATION_TARGET_REPOSITORY] = "Repository",
	[GCLI_NOTIFICATION_TARGET_RELEASE] = "Release",
};

char const *
gcli_notification_target_type_str(enum gcli_notification_target_type type)
{
	if (type > MAX_GCLI_NOTIFICATION_TARGET)
		return NULL;

	return notification_target_type_strings[type];
}

int
gcli_notification_get_issue(struct gcli_ctx *ctx,
                            struct gcli_notification const *const notification,
                            struct gcli_issue *out)
{
	(void) out;

	if (notification->type != GCLI_NOTIFICATION_TARGET_ISSUE) {
		return gcli_error(
			ctx,
			"cannot call gcli_notification_get_issue with a notification of type %s",
			gcli_notification_target_type_str(notification->type));
	}

	gcli_null_check_call(notification_get_issue, ctx, notification, out);
}

int
gcli_notification_get_comments(struct gcli_ctx *ctx,
                               struct gcli_notification const *const notification,
                               struct gcli_comment_list *comments)
{
	gcli_null_check_call(notification_get_comments, ctx, notification, comments);
}
