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

#include <gcli/curl.h>
#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/issues.h>
#include <gcli/gitlab/status.h>
#include <gcli/json_util.h>

#include <sn/sn.h>
#include <pdjson/pdjson.h>

#include <templates/gitlab/status.h>

int
gitlab_get_notifications(struct gcli_ctx *ctx, int const max,
                         struct gcli_notification_list *const out)
{
	char *url = NULL;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->notifications,
		.sizep = &out->notifications_size,
		.parse = (parsefn)(parse_gitlab_todos),
		.max = max,
	};

	url = sn_asprintf("%s/todos", gcli_get_apibase(ctx));

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_notification_mark_as_read(struct gcli_ctx *ctx, char const *id)
{
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/todos/%s/mark_as_done", gcli_get_apibase(ctx), id);
	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_notification_get_issue(struct gcli_ctx *ctx,
                              struct gcli_notification const *const notification,
                              struct gcli_issue *out)
{
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%"PRIid"/issues/%"PRIid,
	                  gcli_get_apibase(ctx), notification->target.project_id,
	                  notification->target.id);

	rc = gitlab_fetch_issue(ctx, url, out);
	free(url);

	return rc;
}

static char const *const gitlab_target_type_names[MAX_GCLI_NOTIFICATION_TARGET] =
{
	[GCLI_NOTIFICATION_TARGET_ISSUE] = "issues",
	[GCLI_NOTIFICATION_TARGET_PULL_REQUEST] = "merge_requests",
};

static int
get_target_type(struct gcli_ctx *ctx,
                struct gcli_notification const *const notification,
                char const **out)
{
	if (notification->type <= GCLI_NOTIFICATION_TARGET_INVALID || notification->type > MAX_GCLI_NOTIFICATION_TARGET)
		return gcli_error(ctx, "notification type is invalid");

	*out = gitlab_target_type_names[notification->type];

	if (*out == NULL) {
		return gcli_error(ctx, "notification type %s is not supported",
		                  gcli_notification_target_type_str(notification->type));
	}

	return 0;
}

int
gitlab_notification_get_comments(struct gcli_ctx *ctx,
                                 struct gcli_notification const *const notification,
                                 struct gcli_comment_list *const out)
{
	int rc = 0;
	char const *kind = NULL;
	char *url;

	rc = get_target_type(ctx, notification, &kind);
	if (rc < 0)
		return rc;

	url = sn_asprintf("%s/projects/%"PRIid"/%s/%"PRIid"/notes",
	                  gcli_get_apibase(ctx), notification->target.project_id,
	                  kind, notification->target.id);

	return gitlab_fetch_comments(ctx, url, out);
}
