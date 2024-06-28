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

#include <err.h>
#include <string.h>

#include <gcli/gcli.h>
#include <gcli/issues.h>
#include <gcli/ctx.h>

#include <templates/bugzilla/bugs.h>

#include <pdjson/pdjson.h>

#include "gcli_tests.h"

static gcli_forge_type
get_bugzilla_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_BUGZILLA;
}

static struct gcli_ctx *
test_context(void)
{
	struct gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_bugzilla_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	r = fopen(p, "r");
	return r;
}

ATF_TC_WITHOUT_HEAD(simple_bugzilla_issue);
ATF_TC_BODY(simple_bugzilla_issue, tc)
{
	struct gcli_issue_list list = {0};
	struct gcli_issue const *issue;
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("bugzilla_simple_bug.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_bugzilla_bugs(ctx, &stream, &list) == 0);

	ATF_REQUIRE_EQ(list.issues_size, 1);

	issue = &list.issues[0];

	ATF_CHECK_EQ(issue->number, 1);
	ATF_CHECK_STREQ(issue->title, "[aha] [scsi] Toshiba MK156FB scsi drive does not work with 2.0 kernel");
	ATF_CHECK_STREQ(issue->created_at, "1994-09-14T09:10:01Z");
	ATF_CHECK_STREQ(issue->author, "Dave Evans");
	ATF_CHECK_STREQ(issue->state, "Closed");
	ATF_CHECK_STREQ(issue->product, "Base System");
	ATF_CHECK_STREQ(issue->component, "kern");

	json_close(&stream);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(bugzilla_comments);
ATF_TC_BODY(bugzilla_comments, tc)
{
	FILE *f;
	struct gcli_comment const *cmt = NULL;
	struct gcli_comment_list list = {0};
	struct gcli_ctx *ctx = test_context();
	struct json_stream stream;

	ATF_REQUIRE(f = open_sample("bugzilla_comments.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_bugzilla_comments(ctx, &stream, &list) == 0);
	json_close(&stream);
	fclose(f);
	f = NULL;

	ATF_REQUIRE_EQ(list.comments_size, 1);

	cmt = &list.comments[0];
	ATF_CHECK_EQ(cmt->id, 1285943);
	ATF_CHECK_STREQ(cmt->author, "zlei@FreeBSD.org");
	ATF_CHECK_STREQ(cmt->date, "2023-11-27T17:20:15Z");
	ATF_CHECK(cmt->body != NULL);

	gcli_comments_free(&list);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(bugzilla_attachments);
ATF_TC_BODY(bugzilla_attachments, tc)
{
	FILE *f = NULL;
	struct gcli_attachment const *it;
	struct gcli_attachment_list list = {0};
	struct gcli_ctx *ctx = test_context();
	struct json_stream stream = {0};

	ATF_REQUIRE(f = open_sample("bugzilla_attachments.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_bugzilla_bug_attachments(ctx, &stream, &list) == 0);

	ATF_CHECK(list.attachments_size == 2);

	it = list.attachments;
	ATF_CHECK_EQ(it->id, 246131);
	ATF_CHECK_EQ(it->is_obsolete, true);
	ATF_CHECK_STREQ(it->author, "nsonack@outlook.com");
	ATF_CHECK_STREQ(it->content_type, "text/plain");
	ATF_CHECK_STREQ(it->created_at, "2023-11-04T20:19:11Z");
	ATF_CHECK_STREQ(it->file_name, "0001-devel-open62541-Update-to-version-1.3.8.patch");
	ATF_CHECK_STREQ(it->summary, "Patch for updating the port");

	it++;
	ATF_CHECK_EQ(it->id, 246910);
	ATF_CHECK_EQ(it->is_obsolete, false);
	ATF_CHECK_STREQ(it->author, "nsonack@outlook.com");
	ATF_CHECK_STREQ(it->content_type, "text/plain");
	ATF_CHECK_STREQ(it->created_at, "2023-12-08T17:10:06Z");
	ATF_CHECK_STREQ(it->file_name, "0001-devel-open62541-Update-to-version-1.3.8.patch");
	ATF_CHECK_STREQ(it->summary, "Patch v2 (now for version 1.3.9)");

	gcli_attachments_free(&list);

	json_close(&stream);
	fclose(f);
	f = NULL;

	gcli_destroy(&ctx);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_bugzilla_issue);
	ATF_TP_ADD_TC(tp, bugzilla_comments);
	ATF_TP_ADD_TC(tp, bugzilla_attachments);

	return atf_no_error();
}
