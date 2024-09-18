/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/forges.h>
#include <gcli/sshkeys.h>
#include <gcli/cmd/table.h>

int
gcli_sshkeys_get_keys(struct gcli_ctx *ctx, struct gcli_sshkey_list *out)
{
	gcli_null_check_call(get_sshkeys, ctx, out);
}

void
gcli_sshkeys_free_keys(struct gcli_sshkey_list *list)
{
	for (size_t i = 0; i < list->keys_size; ++i) {
		free(list->keys[i].title);
		free(list->keys[i].key);
	}

	free(list->keys);

	list->keys = NULL;
	list->keys_size = 0;
}

int
gcli_sshkeys_add_key(struct gcli_ctx *ctx, char const *title,
                     char const *public_key_path, struct gcli_sshkey *out)
{
	int rc;
	char *buffer;
	struct gcli_forge_descriptor const *const forge = gcli_forge(ctx);

	if (forge->add_sshkey == NULL) {
		return gcli_error(ctx, "ssh_add_key is not supported by this forge");
	}

	rc = sn_read_file(public_key_path, &buffer);
	if (rc < 0)
		return rc;

	rc = forge->add_sshkey(ctx, title, buffer, out);
	free(buffer);

	return rc;
}

int
gcli_sshkeys_delete_key(struct gcli_ctx *ctx, gcli_id const id)
{
	gcli_null_check_call(delete_sshkey, ctx, id);
}
