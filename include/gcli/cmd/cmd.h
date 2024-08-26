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

#ifndef GCLI_CMD_CMD_H
#define GCLI_CMD_CMD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <gcli/gcli.h>

#include <sn/sn.h>

extern struct gcli_ctx *g_clictx;

static inline char *
shift(int *argc, char ***argv)
{
	if (*argc == 0)
		errx(1, "error: Not enough arguments");

	(*argc)--;
	return *((*argv)++);
}

void version(void);
void longversion(void);
void copyright(void);
void check_owner_and_repo(const char **owner, const char **repo);

void parse_labels_options(
	int *argc, char ***argv,
	const char ***_add_labels, size_t *_add_labels_size,
	const char ***_remove_labels, size_t *_remove_labels_size);

void delete_repo(bool always_yes, const char *owner, const char *repo);

/* List of subcommand entry points */
int subcommand_api(int argc, char *argv[]);

void gcli_pretty_print(char const *input, int indent, int maxlinelen,
                       FILE *stream);

#endif /* GCLI_CMD_CMD_H */
