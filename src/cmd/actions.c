/*
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/actions.h>
#include <gcli/cmd/cmd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct gcli_cmd_action const *
find_action(struct gcli_cmd_actions const *const actions,
            char const *const action_name)
{
	struct gcli_cmd_action const *a = NULL;

	for (a = actions->defs; a->name != NULL; a += 1) {
		if (strcmp(a->name, action_name) == 0) {
			return a;
		}
	}

	return NULL;
}

int
gcli_cmd_actions_handle(struct gcli_cmd_actions const *const actions,
                        struct gcli_path const *const path,
                        int *argc, char **argv[])
{
	void *item = NULL;
	int rc = 0;

	if (*argc < 1) {
		fprintf(stderr, "gcli: error: missing action\n");
		return 1;
	}

	/* check until we don't have any more remaining arguments */
	for (;;) {

		/* fetch the action name */
		char *const action_name = (*argv)[0];

		/* look for an action definition */
		struct gcli_cmd_action const *const action = find_action(
			actions, action_name);

		if (action == NULL) {
			fprintf(stderr, "gcli: error: unknown action '%s'\n",
			        action_name);

			rc = GCLI_EX_USAGE;
			break;
		}

		/* check whether we need to fetch the item */
		if (action->needs_item && item == NULL) {
			item = calloc(1, actions->item_size);
			if (item == NULL) {
				err(1, "calloc failed");
			}

			rc = actions->fetch_item(g_clictx, path, item);
			if (rc < 0) {
				fprintf(stderr, "gcli: error: failed to fetch: %s\n",
				        gcli_get_error(g_clictx));

				rc = GCLI_EX_DATAERR;
				break;
			}
		}

		/* handle the action */
		rc = action->handler(path, item, argc, argv);
		if (rc < 0) {
			fprintf(stderr, "gcli: action %s failed\n", action_name);
			break;
		}

		shift(argc, argv);

		if (*argc == 0)
			break;

		fputc('\n', stdout);
	}

	if (item) {
		actions->free_item(item);
		free(item);
		item = NULL;
	}

	return rc;
}
