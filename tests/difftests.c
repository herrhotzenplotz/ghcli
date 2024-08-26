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

ATF_TC_WITHOUT_HEAD(free_patch_cleans_up_properly);
ATF_TC_BODY(free_patch_cleans_up_properly, tc)
{
	struct gcli_patch patch = {0};

	gcli_free_patch(&patch);

	ATF_CHECK(patch.prelude == NULL);
	ATF_CHECK(TAILQ_EMPTY(&patch.diffs));
}

ATF_TC_WITHOUT_HEAD(patch_prelude);
ATF_TC_BODY(patch_prelude, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	char const *const fname = "01_diff_prelude.patch";

	FILE *inf = open_sample("01_diff_prelude.patch");
	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_patch_parse_prelude(&parser, &patch) == 0);

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
	ATF_REQUIRE(patch.prelude != NULL);
	ATF_CHECK_STREQ(patch.prelude, expected_prelude);

	free(patch.prelude);

	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(empty_patch_should_not_fail);
ATF_TC_BODY(empty_patch_should_not_fail, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};

	char zeros[] = "";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(zeros, sizeof zeros, "zeros", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);
	ATF_REQUIRE(patch.prelude != NULL);
	ATF_CHECK_STREQ(patch.prelude, "");

	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(empty_hunk_should_not_fault);
ATF_TC_BODY(empty_hunk_should_not_fault, tc)
{
	struct gcli_diff diff = {0};
	struct gcli_diff_parser parser = {0};

	char input[] = "";
	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "testinput", &parser) == 0);

	/* Expect this to error out because there is no diff --git marker */
	ATF_REQUIRE(gcli_parse_diff(&parser, &diff) < 0);

	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(parse_simple_diff);
ATF_TC_BODY(parse_simple_diff, tc)
{
	struct gcli_diff diff = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff_hunk *hunk = NULL;

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
	ATF_REQUIRE(gcli_parse_diff(&parser, &diff) == 0);

	ATF_CHECK_STREQ(diff.file_a, "README");
	ATF_CHECK_STREQ(diff.file_b, "README");

	ATF_CHECK_STREQ(diff.hash_a, "8befdf0");
	ATF_CHECK_STREQ(diff.hash_b, "d193b83");
	ATF_CHECK_STREQ(diff.file_mode, "100644");

	ATF_CHECK_STREQ(diff.r_file, "README");
	ATF_CHECK_STREQ(diff.a_file, "README");

	/* Complete parse */
	ATF_CHECK(parser.hd[0] == '\0');

	/* Check hunks */
	hunk = TAILQ_FIRST(&diff.hunks);
	ATF_REQUIRE(hunk != NULL);

	ATF_CHECK(hunk->range_a_start == 3);
	ATF_CHECK(hunk->range_a_length == 5);
	ATF_CHECK(hunk->range_r_start == 3);
	ATF_CHECK(hunk->range_r_length == 3);
	ATF_CHECK(hunk->diff_line_offset == 1);
	ATF_CHECK_STREQ(hunk->context_info, "This is just a placeholder");
	ATF_CHECK_STREQ(hunk->body,
	                " Test test test\n"
	                " \n"
	                " foo\n"
	                "+\n"
	                "+Hello World\n");

	/* This is the end of the list of hunks */
	hunk = TAILQ_NEXT(hunk, next);
	ATF_CHECK(hunk == NULL);

	gcli_free_diff(&diff);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(diff_with_two_hunks);
ATF_TC_BODY(diff_with_two_hunks, tp)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_diff diff = {0};
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
	ATF_REQUIRE(gcli_parse_diff(&parser, &diff) == 0);

	ATF_CHECK_STREQ(diff.file_a, "README");
	ATF_CHECK_STREQ(diff.file_b, "README");
	ATF_CHECK_STREQ(diff.hash_a, "d193b83");
	ATF_CHECK_STREQ(diff.hash_b, "21af54a");

	ATF_CHECK_STREQ(diff.file_mode, "100644");

	ATF_CHECK_STREQ(diff.r_file, "README");
	ATF_CHECK_STREQ(diff.a_file, "README");

	struct gcli_diff_hunk *h = NULL;

	/* First hunk of this diff */
	h = TAILQ_FIRST(&diff.hunks);
	ATF_REQUIRE(h != NULL);

	ATF_CHECK(h->range_r_start == 1);
	ATF_CHECK(h->range_r_length == 3);
	ATF_CHECK(h->range_a_start == 1);
	ATF_CHECK(h->range_a_length == 5);
	ATF_CHECK(h->diff_line_offset == 1);

	ATF_CHECK_STREQ(h->context_info, "");
	ATF_CHECK_STREQ(h->body,
	                "+Hunk 1\n"
	                "+\n"
	                " This is just a placeholder\n"
	                " \n"
	                " Test test test\n");

	/* Second hunk */
	h = TAILQ_NEXT(h, next);
	ATF_REQUIRE(h != NULL);

	ATF_CHECK(h->range_r_start == 5);
	ATF_CHECK(h->range_r_length == 3);
	ATF_CHECK(h->range_a_start == 7);
	ATF_CHECK(h->range_a_length == 5);
	ATF_CHECK(h->diff_line_offset == 7);

	ATF_CHECK_STREQ(h->context_info, "Test test test");
	ATF_CHECK_STREQ(h->body,
	                " fooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooobar\n"
	                " \n"
	                " Hello World\n"
	                "+\n"
	                "+Hunk 2\n"
	                " \n");

	/* This must be the end of the hunks */
	h = TAILQ_NEXT(h, next);
	ATF_CHECK(h == NULL);

	gcli_free_diff(&diff);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(two_diffs_with_one_hunk_each);
ATF_TC_BODY(two_diffs_with_one_hunk_each, tc)
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
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff *diff;
	struct gcli_diff_hunk *hunk;

	ATF_REQUIRE(gcli_diff_parser_from_buffer(
		diff_data, sizeof(diff_data), "diff_data", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	diff = TAILQ_FIRST(&patch.diffs);
	ATF_REQUIRE(diff != NULL);
	ATF_CHECK_STREQ(diff->file_a, "README");
	ATF_CHECK_STREQ(diff->file_b, "README");
	ATF_CHECK_STREQ(diff->hash_a, "d193b83");
	ATF_CHECK_STREQ(diff->hash_b, "ad32368");
	ATF_CHECK_STREQ(diff->file_mode, "100644");
	ATF_CHECK_STREQ(diff->r_file, "README");
	ATF_CHECK_STREQ(diff->a_file, "README");
	ATF_CHECK(diff->new_file_mode == 0);

	hunk = TAILQ_FIRST(&diff->hunks);
	ATF_REQUIRE(hunk != NULL);
	ATF_CHECK_STREQ(hunk->context_info, "");
	ATF_CHECK(hunk->range_r_start == 1);
	ATF_CHECK(hunk->range_r_length == 3);
	ATF_CHECK(hunk->range_a_start == 1);
	ATF_CHECK(hunk->range_a_length == 5);
	ATF_CHECK(hunk->diff_line_offset == 1);

	ATF_CHECK_STREQ(hunk->body,
	                "+Hunk 1\n"
	                "+\n"
	                " This is just a placeholder\n"
	                " \n"
	                " Test test test\n");

	hunk = TAILQ_NEXT(hunk, next);
	ATF_CHECK(hunk == NULL); /* last one in this list */

	/* Second diff */
	diff = TAILQ_NEXT(diff, next);
	ATF_REQUIRE(diff != NULL);

	ATF_CHECK_STREQ(diff->file_a, "foo.json");
	ATF_CHECK_STREQ(diff->file_b, "foo.json");
	ATF_CHECK_STREQ(diff->hash_a, "0000000");
	ATF_CHECK_STREQ(diff->hash_b, "3be9217");
	ATF_CHECK_STREQ(diff->r_file, "/dev/null");
	ATF_CHECK_STREQ(diff->a_file, "foo.json");
	ATF_CHECK(diff->new_file_mode == 0100644);

	hunk = TAILQ_FIRST(&diff->hunks);
	ATF_REQUIRE(hunk != NULL);
	ATF_CHECK(hunk->range_r_start == 0);
	ATF_CHECK(hunk->range_r_length == 0);
	ATF_CHECK(hunk->range_a_start == 1);
	ATF_CHECK(hunk->range_a_length == 0);
	ATF_CHECK(hunk->diff_line_offset == 1);
	ATF_CHECK_STREQ(hunk->body, "+wat\n");

	/* This must be the last hunk in the diff */
	hunk = TAILQ_NEXT(hunk, next);
	ATF_CHECK(hunk == NULL);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(full_patch);
ATF_TC_BODY(full_patch, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	char const *const fname = "01_diff_prelude.patch";

	FILE *inf = open_sample("01_diff_prelude.patch");
	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(simple_patch_with_comments);
ATF_TC_BODY(simple_patch_with_comments, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff_comments comments = {0};

	char const *const fname = "simple_patch_with_comments.patch";

	FILE *inf = open_sample(fname);
	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	{
		struct gcli_diff_comment *comment = TAILQ_FIRST(&comments);
		ATF_REQUIRE(comment != NULL);

		ATF_CHECK_STREQ(comment->after.filename, "include/ghcli/pulls.h");
		ATF_CHECK(comment->after.start_row == 60);
		ATF_CHECK(comment->diff_line_offset == 4);
		ATF_CHECK_STREQ(comment->comment, "This is a comment on line 60.\n");
		ATF_CHECK_STREQ(comment->diff_text,
		                "+void ghcli_pr_submit(const char *from, const char *to,"
		                " int in_draft);\n");

		comment = TAILQ_NEXT(comment, next);
		ATF_CHECK(comment == NULL);
	}

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(diff_with_two_hunks_and_comments);
ATF_TC_BODY(diff_with_two_hunks_and_comments, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff_comments comments = {0};

	char const input[] =
		"diff --git a/README b/README\n"
		"index d193b83..ad32368 100644\n"
		"--- a/README\n"
		"+++ b/README\n"
		"@@ -1,5 +1,6 @@\n"
		" line 1\n"
		" line 2\n"
		"+new line here\n"
		"This is the first comment\n"
		" line 3\n"
		" \n"
		" \n"
		"@@ -18,4 +19,5 @@\n"
		" \n"
		" line 19\n"
		" line 20\n"
		"This is the other comment\n"
		"+another addition right here\n"
		" line 21\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof(input), "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	{
		struct gcli_diff_comment *comment;

		comment = TAILQ_FIRST(&comments);
		ATF_REQUIRE(comment != NULL);

		ATF_CHECK_STREQ(comment->after.filename, "README");
		ATF_CHECK_STREQ(comment->comment, "This is the first comment\n");
		ATF_CHECK(comment->after.start_row == 4);
		ATF_CHECK(comment->diff_line_offset == 4);

		comment = TAILQ_NEXT(comment, next);
		ATF_REQUIRE(comment != NULL);

		ATF_CHECK_STREQ(comment->after.filename, "README");
		ATF_CHECK_STREQ(comment->comment, "This is the other comment\n");
		ATF_CHECK(comment->after.start_row == 22);
		ATF_CHECK(comment->diff_line_offset == 11);

		comment = TAILQ_NEXT(comment, next);
		ATF_REQUIRE(comment == NULL);
	}

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(patch_with_two_diffs_and_comments);
ATF_TC_BODY(patch_with_two_diffs_and_comments, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff_comments comments = {0};

	char const input[] =
		"diff --git a/bar b/bar\n"
		"index 6c31faf..84b646b 100644\n"
		"--- a/bar\n"
		"+++ b/bar\n"
		"@@ -20,5 +20,5 @@ line 4\n"
		" \n"
		" \n"
		" \n"
		"I do not like this change.\n"
		"-line 5\n"
		"+line 69\n"
		" line 6\n"
		"diff --git a/foo b/foo\n"
		"index 9c2a709..d719e9c 100644\n"
		"--- a/foo\n"
		"+++ b/foo\n"
		"@@ -2,3 +2,12 @@ line 1\n"
		" line 2\n"
		" line 3\n"
		" line 4\n"
		"+\n"
		"+\n"
		"+\n"
		"+\n"
		"+\n"
		"This is horrible\n"
		"Get some help!\n"
		"+\n"
		"+\n"
		"+\n"
		"+This is a random line.\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof(input), "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	{
		struct gcli_diff_comment *c = NULL;

		/* First comment */
		c = TAILQ_FIRST(&comments);
		ATF_REQUIRE(c != NULL);

		ATF_CHECK_STREQ(c->comment, "I do not like this change.\n");
		ATF_CHECK_STREQ(c->after.filename, "bar");
		ATF_CHECK_STREQ(c->before.filename, "bar");
		ATF_CHECK(c->after.start_row == 23);
		ATF_CHECK(c->after.end_row == 23);
		ATF_CHECK(c->before.start_row == 23);
		ATF_CHECK(c->before.end_row == 23);
		ATF_CHECK(c->diff_line_offset == 4);

		/* Second comment */
		c = TAILQ_NEXT(c, next);
		ATF_REQUIRE(c != NULL);

		ATF_CHECK_STREQ(c->after.filename, "foo");
		ATF_CHECK_STREQ(c->comment, "This is horrible\nGet some help!\n");
		ATF_CHECK(c->after.start_row == 10);
		ATF_CHECK(c->diff_line_offset == 9);

		/* End */
		c = TAILQ_NEXT(c, next);
		ATF_CHECK(c == NULL);
	}
}

ATF_TC_WITHOUT_HEAD(single_diff_with_multiline_comment);
ATF_TC_BODY(single_diff_with_multiline_comment, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *c;

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/include/ghcli/pulls.h\n"
		"+++ b/include/ghcli/pulls.h\n"
		"@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *\n"
		" void ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size);\n"
		" void ghcli_print_pr_diff(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" void ghcli_pr_summary(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" \n"
		"This is a comment from line 61 to 62\n"
		"{\n"
		"+void ghcli_pr_submit(const char *from, const char *to, int in_draft);\n"
		" \n"
		"}\n"
		" #endif /* PULLS_H */\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof(input), "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	c = TAILQ_FIRST(&comments);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->after.start_row == 61);
	ATF_CHECK(c->after.end_row == 62);
	ATF_CHECK_STREQ(c->comment, "This is a comment from line 61 to 62\n");

	c = TAILQ_NEXT(c, next);
	ATF_CHECK(c == NULL);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(line_removals_offset_bug);
ATF_TC_BODY(line_removals_offset_bug, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *c;

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/include/ghcli/pulls.h\n"
		"+++ b/include/ghcli/pulls.h\n"
		"@@ -42,4 +42,3 @@ blah\n"
		" \n"
		"Test\n"
		"{\n"
		"-\n"
		"}\n"
		" \n"
		"Another comment\n"
		"{\n"
		" Failure should be here\n"
		"}\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof(input), "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	c = TAILQ_FIRST(&comments);
	ATF_REQUIRE(c != NULL);
	ATF_CHECK(c->after.start_row == 43);
	ATF_CHECK(c->after.end_row == 43);
	ATF_CHECK(c->diff_line_offset == 2);

	c = TAILQ_NEXT(c, next);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->after.start_row == 44);
	ATF_CHECK(c->after.end_row == 44);
	ATF_CHECK(c->diff_line_offset == 5);

	c = TAILQ_NEXT(c, next);
	ATF_CHECK(c == NULL);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(leading_angle_bracket_are_removed_in_comments);
ATF_TC_BODY(leading_angle_bracket_are_removed_in_comments, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *c;

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/include/ghcli/pulls.h\n"
		"+++ b/include/ghcli/pulls.h\n"
		"@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *\n"
		" void ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size);\n"
		" void ghcli_print_pr_diff(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" void ghcli_pr_summary(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" \n"
		"> This is a comment on line 60.\n"
		">\n"
		"> This comment extends over multiple lines.\n"
		"{\n"
		"+void ghcli_pr_submit(const char *from, const char *to, int in_draft);\n"
		" \n"
		"}\n"
		" #endif /* PULLS_H */\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	c = TAILQ_FIRST(&comments);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK_STREQ(c->comment,
	                "This is a comment on line 60.\n"
	                "\n"
	                "This comment extends over multiple lines.\n");

	ATF_CHECK_STREQ(c->diff_text,
	                "+void ghcli_pr_submit(const char *from, const char *to, int in_draft);\n"
	                " \n");

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

static void
get_diff_comments(char const *const in, size_t const in_size,
                  struct gcli_diff_comments *out)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};

	ATF_REQUIRE(gcli_diff_parser_from_buffer(in, in_size, "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(out);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, out) == 0);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(old_and_new_are_set_correctly_in_patch);
ATF_TC_BODY(old_and_new_are_set_correctly_in_patch, tc)
{
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *c;

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/include/ghcli/pulls.h\n"
		"+++ b/include/ghcli/pulls.h\n"
		"@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *\n"
		" void ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size);\n"
		" void ghcli_print_pr_diff(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" void ghcli_pr_summary(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" \n"
		"> This is a comment on line 60.\n"
		">\n"
		"> This comment extends over multiple lines.\n"
		"{\n"
		"+void ghcli_pr_submit(const char *from, const char *to, int in_draft);\n"
		"}\n"
		" #endif /* PULLS_H */\n";

	get_diff_comments(input, sizeof(input), &comments);

	c = TAILQ_FIRST(&comments);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->before.start_row == 61);
	ATF_CHECK(c->before.end_row == 61);
	ATF_CHECK(c->after.start_row == 61);
	ATF_CHECK(c->after.end_row == 61);

	ATF_CHECK(c->start_is_in_new == true);
	ATF_CHECK(c->end_is_in_new == true);
}

ATF_TC_WITHOUT_HEAD(new_and_old_with_both_deletions_and_additions);
ATF_TC_BODY(new_and_old_with_both_deletions_and_additions, tc)
{
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *c;

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/README.md\n"
		"+++ b/README.md\n"
		"@@ -6,9 +6,8 @@ Das hier ist nur ein kurzer Test.\n"
		" Ich füge zum Test hier mal eine neue Zeile ein.\n"
		" \n"
		" \n"
		"> The hell?\n"
		"{\n"
		"-\n"
		"-\n"
		"-\n"
		"+This is just a change.\n"
		"+Across multiple lines.\n"
		"}\n"
		" \n"
		" \n"
		" This line belongs to a different commit.\n";

	get_diff_comments(input, sizeof(input), &comments);

	c = TAILQ_FIRST(&comments);
	ATF_REQUIRE(c != NULL);

	ATF_CHECK(c->before.start_row == 9);
	ATF_CHECK(c->before.end_row == 11);
	ATF_CHECK(c->after.start_row == 9);
	ATF_CHECK(c->after.end_row == 10);

	ATF_CHECK(c->start_is_in_new == false);
	ATF_CHECK(c->end_is_in_new == true);
}

ATF_TC_WITHOUT_HEAD(comment_before_hunk_header);
ATF_TC_BODY(comment_before_hunk_header, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};
	struct gcli_diff_comments comments = {0};

	char const input[] =
		"diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/include/ghcli/pulls.h\n"
		"+++ b/include/ghcli/pulls.h\n"
		"@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *\n"
		" void ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size);\n"
		" void ghcli_print_pr_diff(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" void ghcli_pr_summary(FILE *stream, const char *org, const char *reponame, int pr_number);\n"
		" \n"
		"> Comment here makes no sense whatsoever\n"
		"@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_CHECK(gcli_patch_get_comments(&patch, &comments) < 0);
}

ATF_TC_WITHOUT_HEAD(simple_patch_series);
ATF_TC_BODY(simple_patch_series, tc)
{
	struct gcli_diff_comment *comment;
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_patch *patch;
	struct gcli_patch_series series = {0};
	char const *const fname = "simple_patch_series.patch";

	FILE *inf = open_sample(fname);

	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch_series(&parser, &series) == 0);

	ATF_REQUIRE(patch = TAILQ_FIRST(&series.patches));

	ATF_CHECK_STREQ(patch->prelude,
	                "From 361f83923b9924a3e8796b0ddf03f768e26a1236 Mon Sep 17 00:00:00 2001\n"
	                "From: Nico Sonack <nsonack@herrhotzenplotz.de>\n"
	                "Date: Sat, 16 Sep 2023 22:28:33 +0200\n"
	                "Subject: [PATCH 1/2] Update README.md\n"
	                "\n"
	                "---\n"
	                " README.md | 3 +++\n"
	                " 1 file changed, 3 insertions(+)\n"
	                "\n");

	ATF_REQUIRE(patch = TAILQ_NEXT(patch, next));
	ATF_CHECK_STREQ(patch->prelude,
	                "From d9cbace712a92fdd0bac4f08b6d42e75069af363 Mon Sep 17 00:00:00 2001\n"
	                "From: Nico Sonack <nsonack@herrhotzenplotz.de>\n"
	                "Date: Wed, 20 Sep 2023 20:09:58 +0200\n"
	                "Subject: [PATCH 2/2] Second commit\n"
	                "\n"
	                "This is the body of the commit.\n"
	                "---\n"
	                " README.md | 8 ++++++++\n"
	                " 1 file changed, 8 insertions(+)\n"
	                "\n");

	ATF_CHECK((patch = TAILQ_NEXT(patch, next)) == NULL);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_series_get_comments(&series, &comments) == 0);

	ATF_REQUIRE(comment = TAILQ_FIRST(&comments));

	ATF_CHECK_STREQ(comment->comment, "Why so much whitespace?\n");
	ATF_CHECK_STREQ(comment->diff_text, "+\n+\n");
	ATF_CHECK(comment->after.start_row == 4);
	ATF_CHECK(comment->after.end_row == 5);
	ATF_CHECK(comment->before.start_row == 4);
	ATF_CHECK(comment->before.end_row == 4);

	ATF_REQUIRE(comment = TAILQ_NEXT(comment, next));

	ATF_CHECK_STREQ(comment->comment, "Why all this whitespace?\n");
	ATF_CHECK_STREQ(comment->diff_text, "+\n+\n+\n+\n+\n+\n+\n");
	ATF_CHECK(comment->after.start_row == 7);
	ATF_CHECK(comment->after.end_row == 13);
	ATF_CHECK(comment->before.start_row == 7);
	ATF_CHECK(comment->before.end_row == 7);
}

ATF_TC_WITHOUT_HEAD(patch_series_with_prelude);
ATF_TC_BODY(patch_series_with_prelude, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch_series series = {0};
	char const *const fname = "simple_patch_series.patch";

	FILE *inf = open_sample(fname);

	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch_series(&parser, &series) == 0);

	ATF_CHECK_STREQ(series.prelude,
	                "GCLI: base_sha f00b4rc01dc0fee\n"
	                "This is just a global comment.\n"
	                "\n"
	                "It should not end up in the patch prelude but in the patch series\n"
	                "prelude.\n");
}

/* Git object format version 1 is using SHA256 to hash objects. */
ATF_TC_WITHOUT_HEAD(patch_for_git_object_format_version_1);
ATF_TC_BODY(patch_for_git_object_format_version_1, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch_series series = {0};
	struct gcli_patch *patch;
	char const *const fname = "version_1_object_format.patch";

	FILE *inf = open_sample(fname);

	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch_series(&parser, &series) == 0);

	ATF_REQUIRE(patch = TAILQ_FIRST(&series.patches));
	ATF_CHECK_STREQ(
		patch->commit_hash,
		"a4545b5e32af1be6ba8f41a80dc885ce6c34d36aa5958dfba05b79ffeef8a084");
}

ATF_TC_WITHOUT_HEAD(multiline_change_with_comment);
ATF_TC_BODY(multiline_change_with_comment, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *comment;

	char const *const fname = "multiline_change_with_comment.diff";

	FILE *inf = open_sample(fname);

	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	ATF_REQUIRE(comment = TAILQ_FIRST(&comments));
	ATF_CHECK(comment->before.start_row == 9);
	ATF_CHECK(comment->before.end_row == 11);
	ATF_CHECK(comment->after.start_row == 9);
	ATF_CHECK(comment->after.end_row == 10);
}

ATF_TC_WITHOUT_HEAD(bug_patch_series_fail_get_comments);
ATF_TC_BODY(bug_patch_series_fail_get_comments, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch_series series = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_patch const *p = NULL;

	char const *const fname = "patch_series_fail_get_comments.patch";

	FILE *inf = open_sample(fname);

	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch_series(&parser, &series) == 0);

	p = TAILQ_FIRST(&series.patches);
	{
		struct gcli_diff const *d = TAILQ_FIRST(&p->diffs);
		struct gcli_diff_hunk const *h = TAILQ_FIRST(&d->hunks);

		ATF_CHECK_STREQ(h->body,
		                " # README\n"
		                " \n"
		                " Das hier ist nur ein kurzer Test.\n"
		                "Deine Mutter\n"
		                "{\n"
		                "+\n"
		                "+\n"
		                "+Ich füge zum Test hier mal eine neue Zeile ein.\n"
		                "}\n");

		ATF_CHECK(TAILQ_NEXT(h, next) == NULL);
	}

	p = TAILQ_NEXT(p, next);
	{
		struct gcli_diff const *d = TAILQ_FIRST(&p->diffs);
		struct gcli_diff_hunk const *h = TAILQ_FIRST(&d->hunks);

		ATF_CHECK_STREQ(h->body,
		                " \n"
		                " \n"
		                " Ich füge zum Test hier mal eine neue Zeile ein.\n"
		                "+\n"
		                "+\n"
		                "+\n"
		                "+\n"
		                "+\n"
		                "+\n"
		                "+\n"
		                "Naja...\n"
		                "{\n"
		                "+This line belongs to a different commit.\n"
		                "}\n");

		ATF_CHECK(TAILQ_NEXT(h, next) == NULL);
	}

	p = TAILQ_NEXT(p, next);
	{
		struct gcli_diff const *d = TAILQ_FIRST(&p->diffs);
		struct gcli_diff_hunk const *h = TAILQ_FIRST(&d->hunks);

		ATF_CHECK_STREQ(h->body,
		                " Ich füge zum Test hier mal eine neue Zeile ein.\n"
		                " \n"
		                " \n"
		                "-\n"
		                "-\n"
		                "-\n"
		                "+This is just a change.\n"
		                "+Across multiple lines.\n"
		                " \n"
		                " \n"
		                " This line belongs to a different commit.\n");

		ATF_CHECK(TAILQ_NEXT(h, next) == NULL);
	}

	ATF_REQUIRE(gcli_patch_series_get_comments(&series, &comments) == 0);
}

ATF_TC_WITHOUT_HEAD(bug_short_hunk_range);
ATF_TC_BODY(bug_short_hunk_range, tc)
{
	struct gcli_diff_parser parser = {0};
	struct gcli_patch patch = {0};

	char const input[] =
		"diff --git a/foo b/foo\n"
		"index 30a503cf..05d233eb 100644\n"
		"--- a/foo\n"
		"+++ b/foo\n"
		"@@ -1 +1 @@\n"
		"-wat\n"
		"+banana\n";

	ATF_REQUIRE(gcli_diff_parser_from_buffer(input, sizeof input, "input", &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TC_WITHOUT_HEAD(bug_no_newline_at_end_of_file);
ATF_TC_BODY(bug_no_newline_at_end_of_file, tc)
{
	struct gcli_patch patch = {0};
	struct gcli_diff_parser parser = {0};
	struct gcli_diff_comments comments = {0};
	struct gcli_diff_comment *comment = NULL;

	char const *const fname = "stuff_with_no_newline_in_diff.diff";

	FILE *inf = open_sample(fname);
	ATF_REQUIRE(gcli_diff_parser_from_file(inf, fname, &parser) == 0);
	ATF_REQUIRE(gcli_parse_patch(&parser, &patch) == 0);

	TAILQ_INIT(&comments);
	ATF_REQUIRE(gcli_patch_get_comments(&patch, &comments) == 0);

	comment = TAILQ_FIRST(&comments);
	ATF_REQUIRE(comment);

	ATF_CHECK(comment->before.start_row == 1);
	ATF_CHECK(comment->before.end_row == 1);

	ATF_CHECK(comment->after.start_row == 1);
	ATF_CHECK(comment->after.end_row == 1);

	ATF_CHECK_STREQ(comment->comment, "This is a comment\n");
	ATF_CHECK_STREQ(comment->diff_text,
	                "-this is a test file\n"
	                "+this is a test file\n"
	                "\\ No newline at end of file\n");

	gcli_free_patch(&patch);
	gcli_free_diff_parser(&parser);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, free_patch_cleans_up_properly);
	ATF_TP_ADD_TC(tp, patch_prelude);
	ATF_TP_ADD_TC(tp, empty_patch_should_not_fail);
	ATF_TP_ADD_TC(tp, parse_simple_diff);
	ATF_TP_ADD_TC(tp, empty_hunk_should_not_fault);
	ATF_TP_ADD_TC(tp, diff_with_two_hunks);
	ATF_TP_ADD_TC(tp, two_diffs_with_one_hunk_each);
	ATF_TP_ADD_TC(tp, full_patch);
	ATF_TP_ADD_TC(tp, simple_patch_with_comments);
	ATF_TP_ADD_TC(tp, diff_with_two_hunks_and_comments);
	ATF_TP_ADD_TC(tp, patch_with_two_diffs_and_comments);
	ATF_TP_ADD_TC(tp, single_diff_with_multiline_comment);
	ATF_TP_ADD_TC(tp, line_removals_offset_bug);
	ATF_TP_ADD_TC(tp, leading_angle_bracket_are_removed_in_comments);
	ATF_TP_ADD_TC(tp, comment_before_hunk_header);
	ATF_TP_ADD_TC(tp, simple_patch_series);
	ATF_TP_ADD_TC(tp, patch_series_with_prelude);
	ATF_TP_ADD_TC(tp, multiline_change_with_comment);
	ATF_TP_ADD_TC(tp, old_and_new_are_set_correctly_in_patch);
	ATF_TP_ADD_TC(tp, new_and_old_with_both_deletions_and_additions);
	ATF_TP_ADD_TC(tp, patch_for_git_object_format_version_1);
	ATF_TP_ADD_TC(tp, bug_patch_series_fail_get_comments);
	ATF_TP_ADD_TC(tp, bug_short_hunk_range);
	ATF_TP_ADD_TC(tp, bug_no_newline_at_end_of_file);

	return atf_no_error();
}
