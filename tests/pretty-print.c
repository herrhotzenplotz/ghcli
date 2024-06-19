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

#include <sn/sn.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

static char *
test_string(char const *const input)
{
	FILE *f;
	long length;
	char *buf;
	static char const *const fname = "/tmp/gcli_pretty_print_test";

	f = fopen(fname, "w");
	pretty_print(input, 4, 80, f);

	length = ftell(f);
	fflush(f);
	fclose(f);

	f = fopen(fname, "r");
	buf = malloc(length + 1);
	fread(buf, 1, length, f);
	buf[length] = '\0';

	fclose(f);
	unlink(fname);

	return buf;
}

ATF_TC_WITHOUT_HEAD(simple_one_line);
ATF_TC_BODY(simple_one_line, tc)
{
	char *formatted = test_string("<empty>");
	ATF_CHECK_STREQ(formatted, "    <empty>\n");
}

ATF_TC_WITHOUT_HEAD(long_line_doesnt_break);
ATF_TC_BODY(long_line_doesnt_break, tc)
{
	char const *const input =
		"0123456789012345678901234567890123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789012345678901234567890123456789";
	char *formatted = test_string(input);
	char const expected[] =
		"    0123456789012345678901234567890123456789012345678901234567890123456789012345"
		"6789012345678901234567890123456789012345678901234567890123456789\n";

	ATF_CHECK_STREQ(formatted, expected);
}

ATF_TC_WITHOUT_HEAD(line_overflow_break);
ATF_TC_BODY(line_overflow_break, tc)
{
	char const *const input =
		"0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 "
		"0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789";
	char *formatted = test_string(input);
	char const expected[] =
		"    0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 \n"
		"    0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 0123456789 \n"
		"    0123456789 0123456789\n";
	ATF_CHECK_STREQ(formatted, expected);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_one_line);
	ATF_TP_ADD_TC(tp, long_line_doesnt_break);
	ATF_TP_ADD_TC(tp, line_overflow_break);

	return atf_no_error();
}
