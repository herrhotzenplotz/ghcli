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

#include <gcli/cmd/open.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>

#include <gcli/waitproc.h>

#include <sn/sn.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
gcli_cmd_open_url(char const *const url)
{
	pid_t p;
	int rc = 0;
	char *open_program = NULL;

	if (!url) {
		fprintf(stderr, "gcli: error: got no url from forge\n");
		return -1;
	}

	p = fork();
	if (p < 0)
		err(1, "fork");

	/* wait for the child */
	if (p != 0)
		return gcli_wait_proc_ok(g_clictx, p);

	/* look for an open-program */
	open_program = gcli_config_get_url_open_program(g_clictx);
	if (open_program == NULL)
		open_program = "xdg-open";

        rc = execlp(open_program, open_program, url, NULL);
	if (rc < 0)
		exit(EXIT_FAILURE);

	abort();
}
