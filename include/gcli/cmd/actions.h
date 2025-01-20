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

#ifndef GCLI_CMD_ACTIONS_H
#define GCLI_CMD_ACTIONS_H

#include <gcli/path.h>

#include <stdbool.h>
#include <stddef.h>

enum {
	GCLI_EX_OK = 0,
	GCLI_EX_USAGE = 1,
	GCLI_EX_DATAERR = 2,
};

typedef int (*gcli_cmd_action_handler)(
	struct gcli_path const *path,      /* path to the object */
	void *item,                        /* path to the fetched item or NULL */
	int *argc, char **argv[]);         /* argument vector (for incremental getopt parsing) */

typedef int (*gcli_cmd_action_fetcher)(
	struct gcli_ctx *,
	struct gcli_path const *,
	void *item);

typedef void (*gcli_cmd_action_freeer)(void *item);

/* definition of an action */
struct gcli_cmd_action {
	char *name;                        /* name that this action is invoked as */
	bool needs_item;                   /* whether we must pass the fetched item or can just pass a NULL */
	gcli_cmd_action_handler handler;   /* the action handler */
};

/* Maximum number of items */
#define GCLI_ACTION_LIST_MAX 32

struct gcli_cmd_actions {
	gcli_cmd_action_fetcher fetch_item;
	gcli_cmd_action_freeer free_item;
	size_t item_size;

	struct gcli_cmd_action defs[GCLI_ACTION_LIST_MAX];
};

int gcli_cmd_actions_handle(struct gcli_cmd_actions const *actions,
                            struct gcli_path const *path,
                            int *argc, char **argv[]);

#endif /* GCLI_CMD_ACTIONS_H */
