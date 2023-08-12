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

#ifndef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gcli/gitlab/sshkeys.h>
#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <sn/sn.h>
#include <pdjson/pdjson.h>

#include <templates/gitlab/sshkeys.h>

int
gitlab_get_sshkeys(gcli_sshkey_list *list)
{
	char *url, *next_url = NULL;

	*list = (gcli_sshkey_list) {0};
	url = sn_asprintf("%s/user/keys", gcli_get_apibase());

	do {
		gcli_fetch_buffer buf = {0};
		json_stream str;

		gcli_fetch(url, &next_url, &buf);
		json_open_buffer(&str, buf.data, buf.length);
		parse_gitlab_sshkeys(&str, &list->keys, &list->keys_size);

		json_close(&str);
		free(buf.data);
		free(url);
	} while ((url = next_url));

	return 0;
}

int
gitlab_add_sshkey(char const *const title,
                  char const *const pubkey,
                  gcli_sshkey *const out)
{
	char *url, *payload;
	char *e_title, *e_key;
	gcli_fetch_buffer buf = {0};

	url = sn_asprintf("%s/user/keys", gcli_get_apibase());

	/* Prepare payload */
	e_title = gcli_json_escape_cstr(title);
	e_key = gcli_json_escape_cstr(pubkey);
	payload = sn_asprintf(
		"{ \"title\": \"%s\", \"key\": \"%s\" }",
		e_title, e_key);
	free(e_title);
	free(e_key);

	gcli_fetch_with_method("POST", url, payload, NULL, &buf);
	if (out) {
		json_stream str;

		json_open_buffer(&str, buf.data, buf.length);
		parse_gitlab_sshkey(&str, out);
		json_close(&str);
	}

	free(buf.data);

	return 0;
}

int
gitlab_delete_sshkey(int id)
{
	char *url;

	url = sn_asprintf("%s/user/keys/%d", gcli_get_apibase(), id);
	gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);

	return 0;
}