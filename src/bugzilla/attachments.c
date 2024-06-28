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

#include <gcli/bugzilla/attachments.h>

#include <gcli/base64.h>
#include <gcli/curl.h>

#include <templates/bugzilla/bugs.h>

int
bugzilla_attachment_get_content(struct gcli_ctx *ctx, gcli_id attachment_id,
                                FILE *output)
{
	int rc = 0;
	char *url;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	struct gcli_attachment attachment = {0};

	url = sn_asprintf("%s/rest/bug/attachment/%"PRIid,
	                  gcli_get_apibase(ctx), attachment_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_attachment_content(ctx, &stream, &attachment);
	if (rc < 0)
		goto error_parse;

	rc = gcli_base64_decode_print(ctx, output, attachment.data_base64);

	gcli_attachment_free(&attachment);

error_parse:
	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	free(url);

	return rc;
}
