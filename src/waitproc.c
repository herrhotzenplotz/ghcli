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

#include <gcli/gcli.h>

#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int
gcli_wait_proc_ok(struct gcli_ctx *ctx, pid_t pid)
{
	int status;

	if (waitpid(pid, &status, WEXITED) == -1) {
		return gcli_error(ctx, "failed to wait for child process: %s",
		                  strerror(errno));
	}

	if (WIFEXITED(status)) {
		int exit_code = WEXITSTATUS(status);
		if (exit_code) {
			return gcli_error(ctx, "child exited with error code %d",
			                  WEXITSTATUS(status));
		}
		return 0;
	}

	if (WIFSIGNALED(status)) {
		return gcli_error(ctx, "child exited due to signal %d",
		                  WTERMSIG(status));
	}

	return gcli_error(ctx, "unknown child status");
}
