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

#ifndef STATUS_H
#define STATUS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>
#include <gcli/issues.h>

#include <stdlib.h>

enum gcli_notification_target_type {
	GCLI_NOTIFICATION_TARGET_INVALID = 0,

	GCLI_NOTIFICATION_TARGET_ISSUE,
	GCLI_NOTIFICATION_TARGET_PULL_REQUEST,
	GCLI_NOTIFICATION_TARGET_COMMIT,
	GCLI_NOTIFICATION_TARGET_EPIC,
	GCLI_NOTIFICATION_TARGET_REPOSITORY,

	MAX_GCLI_NOTIFICATION_TARGET,
};

struct gcli_notification {
	char *id;
	char *title;
	char *reason;
	char *date;
	enum gcli_notification_target_type type;
	char *repository;

	/* target specific data */
	gcli_id target_id;   /* The internal ID of the target data */
};

struct gcli_notification_list {
	struct gcli_notification *notifications;
	size_t notifications_size;
};

int gcli_get_notifications(struct gcli_ctx *ctx, int count,
                           struct gcli_notification_list *out);
int gcli_notification_mark_as_read(struct gcli_ctx *ctx, char const *id);
void gcli_free_notification(struct gcli_notification *);
void gcli_free_notifications(struct gcli_notification_list *);
char const *gcli_notification_target_type_str(enum gcli_notification_target_type type);
int gcli_notification_get_issue(struct gcli_ctx *ctx,
                                struct gcli_notification const *notification,
                                struct gcli_issue *out);


#endif /* STATUS_H */
