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
#include <gcli/gitlab/checkout.h>
#include <gcli/waitproc.h>

#include <sn/sn.h>

#include <stdlib.h>
#include <unistd.h>

int
gitlab_mr_checkout(struct gcli_ctx *ctx, char const *const remote, gcli_id const pr_id)
{
	/* FIXME: this is more than not ideal! */
	char *remote_ref, *local_ref, *refspec;
	int rc;
	pid_t pid;

	remote_ref = sn_asprintf("merge-requests/%"PRIid"/head", pr_id);
	local_ref = sn_asprintf("gitlab/mr/%"PRIid, pr_id);
	refspec = sn_asprintf("%s:%s", remote_ref, local_ref);

	pid = fork();
	if (pid < 0)
		return gcli_error(ctx, "could not fork");

	if (pid == 0) {
		rc = execlp("git", "git", "fetch", remote, refspec, NULL);
		if (rc < 0)
			exit(EXIT_FAILURE);

		/* NOTREACHED */
	}

	rc = gcli_wait_proc_ok(ctx, pid);
	if (rc < 0)
		return rc;

	free(remote_ref); remote_ref = NULL;
	free(refspec); refspec = NULL;

	pid = fork();
	if (pid < 0)
		return gcli_error(ctx, "could not fork");

	if (pid == 0) {
		rc = execlp("git", "git", "checkout", "--track", local_ref, NULL);
		if (rc < 0)
			exit(EXIT_FAILURE);

		/* NOTREACHED */
	}

	rc = gcli_wait_proc_ok(ctx, pid);

	free(local_ref); local_ref = NULL;

	return rc;
}
