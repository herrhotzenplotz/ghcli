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

#include <atf-c.h>
#include <stdio.h>

#include <config.h>

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	ATF_REQUIRE(r = fopen(p, "r"));

	return r;
}

ATF_TC_WITHOUT_HEAD(free_diff_cleans_up_properly);
ATF_TC_BODY(free_diff_cleans_up_properly, tc)
{
	gcli_diff diff = {0};

	gcli_free_diff(&diff);

	ATF_CHECK(diff.prelude == NULL);
	ATF_CHECK(TAILQ_EMPTY(&diff.hunks));
}

ATF_TC_WITHOUT_HEAD(patch_prelude);
ATF_TC_BODY(patch_prelude, tc)
{
	gcli_diff diff = {0};
	gcli_diff_parser parser = {0};
	char const *const fname = "01_diff_prelude.patch";

	FILE *inf = open_sample("01_diff_prelude.patch");
	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_diff_parse_prelude(&parser, &diff) == 0);

	char const expected_prelude[] =
		"From 47b40f51cae6cec9a3132f888fd66c21ecb687fa Mon Sep 17 00:00:00 2001\n"
		"From: Nico Sonack <nsonack@outlook.com>\n"
		"Date: Sun, 10 Oct 2021 12:23:11 +0200\n"
		"Subject: [PATCH] Start submission implementation\n"
		"\n"
		"---\n"
		" include/ghcli/pulls.h |  1 +\n"
		" src/ghcli.c           | 55 +++++++++++++++++++++++++++++++++++++++++++\n"
		" src/pulls.c           |  9 +++++++\n"
		" 3 files changed, 65 insertions(+)\n"
		"\n";
	ATF_REQUIRE(diff.prelude != NULL);
	ATF_CHECK_STREQ(diff.prelude, expected_prelude);
}

ATF_TC_WITHOUT_HEAD(empty_patch_should_not_fail);
ATF_TC_BODY(empty_patch_should_not_fail, tc)
{
	gcli_diff diff = {0};
	gcli_diff_parser parser = {0};

	char zeros[] = "";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(zeros, sizeof zeros, "zeros", &parser) == 0);
	ATF_REQUIRE(gcli_diff_parse_prelude(&parser, &diff) == 0);
	ATF_REQUIRE(diff.prelude != NULL);
	ATF_CHECK_STREQ(diff.prelude, "");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, free_diff_cleans_up_properly);
	ATF_TP_ADD_TC(tp, patch_prelude);
	ATF_TP_ADD_TC(tp, empty_patch_should_not_fail);
	return atf_no_error();
}
