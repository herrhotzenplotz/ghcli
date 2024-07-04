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

#include <gcli/gitea/config.h>
#include <gcli/gitea/labels.h>
#include <gcli/gitea/repos.h>
#include <gcli/github/labels.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

int
gitea_get_labels(struct gcli_ctx *ctx, struct gcli_path const *const path,
                 int max, struct gcli_label_list *const list)
{
	return github_get_labels(ctx, path, max, list);
}

int
gitea_create_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   struct gcli_label *const label)
{
	return github_create_label(ctx, path, label);
}

int
gitea_delete_label(struct gcli_ctx *ctx,
                   struct gcli_path const *const repo_path, char const *label)
{
	char *url = NULL;
	struct gcli_label_list list = {0};
	int id = -1;
	int rc = 0;

	/* Gitea wants the id of the label, not its name. thus fetch all
	 * the labels first to then find out what the id is we need. */
	rc = gitea_get_labels(ctx, repo_path, -1, &list);
	if (rc < 0)
		return rc;

	/* Search for the id */
	for (size_t i = 0; i < list.labels_size; ++i) {
		if (strcmp(list.labels[i].name, label) == 0) {
			id = list.labels[i].id;
			break;
		}
	}

	gcli_free_labels(&list);

	/* did we find a label? */
	if (id < 0)
		return gcli_error(ctx, "label '%s' does not exist", label);

	/* DELETE /repos/{owner}/{repo}/labels/{} */
	rc = gitea_repo_make_url(ctx, repo_path, &url, "/labels/%d", id);

	if (rc == 0) {
		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);
	}

	free(url);

	return rc;
}
