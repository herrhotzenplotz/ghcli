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

	free(diff.prelude);

	gcli_free_diff_parser(&parser);
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

	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(empty_hunk_should_not_fault);
ATF_TC_BODY(empty_hunk_should_not_fault, tc)
{
	gcli_diff_hunk hunk = {0};
	gcli_diff_parser parser = {0};

	char input[] = "";
	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "testinput", &parser) == 0);

	/* Expect this to error out because there is no diff --git marker */
	ATF_REQUIRE(gcli_diff_parse_hunk(&parser, &hunk) < 0);

	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(parse_simple_diff_hunk);
ATF_TC_BODY(parse_simple_diff_hunk, tc)
{
	gcli_diff_hunk diff_hunk = {0};
	gcli_diff_parser parser = {0};
	gcli_diff_chunk *chunk = NULL;

	char zeros[] =
		"diff --git a/README b/README\n"
		"index 8befdf0..d193b83 100644\n"
		"--- a/README\n"
		"+++ b/README\n"
		"@@ -3,3 +3,5 @@ This is just a placeholder\n"
		" Test test test\n"
		" \n"
		" foo\n"
		"+\n"
		"+Hello World\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(zeros, sizeof zeros, "zeros", &parser) == 0);
	ATF_REQUIRE(gcli_diff_parse_hunk(&parser, &diff_hunk) == 0);

	ATF_CHECK_STREQ(diff_hunk.file_a, "README");
	ATF_CHECK_STREQ(diff_hunk.file_b, "README");

	ATF_CHECK_STREQ(diff_hunk.hash_a, "8befdf0");
	ATF_CHECK_STREQ(diff_hunk.hash_b, "d193b83");
	ATF_CHECK_STREQ(diff_hunk.file_mode, "100644");

	ATF_CHECK_STREQ(diff_hunk.r_file, "README");
	ATF_CHECK_STREQ(diff_hunk.a_file, "README");

	/* Complete parse */
	ATF_CHECK(parser.hd[0] == '\0');

	/* Check chunks */
	chunk = TAILQ_FIRST(&diff_hunk.chunks);
	ATF_REQUIRE(chunk != NULL);

	ATF_CHECK(chunk->range_a_start == 3);
	ATF_CHECK(chunk->range_a_end == 5);
	ATF_CHECK(chunk->range_r_start == 3);
	ATF_CHECK(chunk->range_r_end == 3);
	ATF_CHECK_STREQ(chunk->context_info, "This is just a placeholder");
	ATF_CHECK_STREQ(chunk->body,
	                " Test test test\n"
	                " \n"
	                " foo\n"
	                "+\n"
	                "+Hello World\n");

	/* This is the end of the list of chunks */
	chunk = TAILQ_NEXT(chunk, next);
	ATF_CHECK(chunk == NULL);

	gcli_free_diff_hunk(&diff_hunk);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(hunk_with_two_chunks);
ATF_TC_BODY(hunk_with_two_chunks, tp)
{
	gcli_diff_parser parser = {0};
	gcli_diff_hunk hunk = {0};
	char input[] =
		"diff --git a/README b/README\n"
		"index d193b83..21af54a 100644\n"
		"--- a/README\n"
		"+++ b/README\n"
		"@@ -1,3 +1,5 @@\n"
		"+Hunk 1\n"
		"+\n"
		" This is just a placeholder\n"
		" \n"
		" Test test test\n"
		"@@ -5,3 +7,5 @@ Test test test\n"
		" fooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooobar\n"
		" \n"
		" Hello World\n"
		"+\n"
		"+Hunk 2\n"
		" \n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "<input>", &parser) == 0);
	ATF_REQUIRE(gcli_diff_parse_hunk(&parser, &hunk) == 0);

	ATF_CHECK_STREQ(hunk.file_a, "README");
	ATF_CHECK_STREQ(hunk.file_b, "README");
	ATF_CHECK_STREQ(hunk.hash_a, "d193b83");
	ATF_CHECK_STREQ(hunk.hash_b, "21af54a");

	ATF_CHECK_STREQ(hunk.file_mode, "100644");

	ATF_CHECK_STREQ(hunk.r_file, "README");
	ATF_CHECK_STREQ(hunk.a_file, "README");

	gcli_diff_chunk *c = NULL;

	/* First chunk of this hunk */
	c = TAILQ_FIRST(&hunk.chunks);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->range_r_start == 1);
	ATF_CHECK(c->range_r_end == 3);
	ATF_CHECK(c->range_a_start == 1);
	ATF_CHECK(c->range_a_end == 5);

	ATF_CHECK_STREQ(c->context_info, "");
	ATF_CHECK_STREQ(c->body,
	                "+Hunk 1\n"
	                "+\n"
	                " This is just a placeholder\n"
	                " \n"
	                " Test test test\n");

	/* Second chunk */
	c = TAILQ_NEXT(c, next);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->range_r_start == 5);
	ATF_CHECK(c->range_r_end == 3);
	ATF_CHECK(c->range_a_start == 7);
	ATF_CHECK(c->range_a_end == 5);

	ATF_CHECK_STREQ(c->context_info, "Test test test");
	ATF_CHECK_STREQ(c->body,
	                " fooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooobar\n"
	                " \n"
	                " Hello World\n"
	                "+\n"
	                "+Hunk 2\n"
	                " \n");

	/* This must be the end of the chunks */
	c = TAILQ_NEXT(c, next);
	ATF_CHECK(c == NULL);

	gcli_free_diff_hunk(&hunk);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(two_hunks_with_one_chunk_each);
ATF_TC_BODY(two_hunks_with_one_chunk_each, tc)
{
	char const diff_data[] =
		"diff --git a/README b/README\n"
		"index d193b83..ad32368 100644\n"
		"--- a/README\n"
		"+++ b/README\n"
		"@@ -1,3 +1,5 @@\n"
		"+Hunk 1\n"
		"+\n"
		" This is just a placeholder\n"
		" \n"
		" Test test test\n"
		"diff --git a/foo.json b/foo.json\n"
		"new file mode 100644\n"
		"index 0000000..3be9217\n"
		"--- /dev/null\n"
		"+++ b/foo.json\n"
		"@@ -0,0 +1 @@\n"
		"+wat\n";
	gcli_diff diff = {0};
	gcli_diff_parser parser = {0};
	gcli_diff_hunk *hunk;
	gcli_diff_chunk *chunk;

	ATF_REQUIRE(gcli_diff_parser_from_buffer(diff_data, sizeof(diff_data), "diff_data", &parser) == 0);
	ATF_REQUIRE(gcli_parse_diff(&parser, &diff) == 0);

	hunk = TAILQ_FIRST(&diff.hunks);
	ATF_REQUIRE(hunk != NULL);
	ATF_CHECK_STREQ(hunk->file_a, "README");
	ATF_CHECK_STREQ(hunk->file_b, "README");
	ATF_CHECK_STREQ(hunk->hash_a, "d193b83");
	ATF_CHECK_STREQ(hunk->hash_b, "ad32368");
	ATF_CHECK_STREQ(hunk->file_mode, "100644");
	ATF_CHECK_STREQ(hunk->r_file, "README");
	ATF_CHECK_STREQ(hunk->a_file, "README");
	ATF_CHECK(hunk->new_file_mode == 0);

	chunk = TAILQ_FIRST(&hunk->chunks);
	ATF_REQUIRE(chunk != NULL);
	ATF_CHECK_STREQ(chunk->context_info, "");
	ATF_CHECK(chunk->range_r_start == 1);
	ATF_CHECK(chunk->range_r_end == 3);
	ATF_CHECK(chunk->range_a_start == 1);
	ATF_CHECK(chunk->range_a_end == 5);

	ATF_CHECK_STREQ(chunk->body,
	                "+Hunk 1\n"
	                "+\n"
	                " This is just a placeholder\n"
	                " \n"
	                " Test test test\n");

	chunk = TAILQ_NEXT(chunk, next);
	ATF_CHECK(chunk == NULL); /* last one in this list */

	/* Second hunk */
	hunk = TAILQ_NEXT(hunk, next);
	ATF_REQUIRE(hunk != NULL);

	ATF_CHECK_STREQ(hunk->file_a, "foo.json");
	ATF_CHECK_STREQ(hunk->file_b, "foo.json");
	ATF_CHECK_STREQ(hunk->hash_a, "0000000");
	ATF_CHECK_STREQ(hunk->hash_b, "3be9217");
	ATF_CHECK_STREQ(hunk->r_file, "/dev/null");
	ATF_CHECK_STREQ(hunk->a_file, "foo.json");
	ATF_CHECK(hunk->new_file_mode == 0100644);

	chunk = TAILQ_FIRST(&hunk->chunks);
	ATF_REQUIRE(chunk != NULL);
	ATF_CHECK(chunk->range_r_start == 0);
	ATF_CHECK(chunk->range_r_end == 0);
	ATF_CHECK(chunk->range_a_start == 1);
	ATF_CHECK(chunk->range_a_end == 0);
	ATF_CHECK_STREQ(chunk->body, "+wat\n");

	gcli_free_diff(&diff);
	gcli_free_diff_parser(&parser);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, free_diff_cleans_up_properly);
	ATF_TP_ADD_TC(tp, patch_prelude);
	ATF_TP_ADD_TC(tp, empty_patch_should_not_fail);
	ATF_TP_ADD_TC(tp, parse_simple_diff_hunk);
	ATF_TP_ADD_TC(tp, empty_hunk_should_not_fault);
	ATF_TP_ADD_TC(tp, hunk_with_two_chunks);
	ATF_TP_ADD_TC(tp, two_hunks_with_one_chunk_each);

	return atf_no_error();
}
