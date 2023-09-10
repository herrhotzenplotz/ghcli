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

#include <gcli/diffutil.h>

int
gcli_diff_parser_from_buffer(char *buf, size_t buf_size,
                             char const *filename, gcli_diff_parser *out)
{
	out->buf = out->hd = buf;
	out->buf_size = buf_size;
	out->filename = filename;
	out->col = out->row = 1;

	return 0;
}

int
gcli_diff_parser_from_file(FILE *f, char const *filename,
                           gcli_diff_parser *out)
{
	long len = 0;
	char *buf;

	if (fseek(f, 0, SEEK_END) < 0)
		return -1;

	if ((len = ftell(f)) < 0)
		return -1;

	if (fseek(f, 0, SEEK_SET) < 0)
		return -1;

	buf = malloc(len + 1);
	fread(buf, len, 1, f);

	return gcli_diff_parser_from_buffer(buf, len, filename, out);
}

int
gcli_parse_diff(gcli_diff_parser *parser, gcli_diff *out)
{
	(void) parser;
	(void) out;

	return -1;
}

int
gcli_diff_parse_prelude(gcli_diff_parser *parser, gcli_diff *out)
{
	(void) parser;
	(void) out;

	return 0;
}

void
gcli_free_diff(gcli_diff *diff)
{
	(void) diff;
}
