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

#include <templates/github/checks.h>
#include <templates/github/comments.h>
#include <templates/github/forks.h>
#include <templates/github/issues.h>
#include <templates/github/labels.h>
#include <templates/github/milestones.h>
#include <templates/github/pulls.h>
#include <templates/github/releases.h>
#include <templates/github/repos.h>

#include <err.h>
#include <string.h>

#include <gcli/ctx.h>

#include "gcli_tests.h"

static gcli_forge_type
get_github_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITHUB;
}

static struct gcli_ctx *
test_context(void)
{
	struct gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
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

ATF_TC_WITHOUT_HEAD(simple_github_issue);
ATF_TC_BODY(simple_github_issue, tc)
{
	struct gcli_issue issue = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_issue.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_issue(ctx, &stream, &issue) == 0);

	ATF_CHECK(issue.number = 115);
	ATF_CHECK_STREQ(issue.title, "consider removing FILE *out from printing functions");
	ATF_CHECK_STREQ(issue.created_at, "2022-03-22T16:06:10Z");
	ATF_CHECK_STREQ(issue.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(issue.state, "closed");
	ATF_CHECK(issue.comments == 0);
	ATF_CHECK(issue.locked == false);

	ATF_CHECK_STREQ(issue.body,
		        "We use these functions with ghcli only anyways. In "
		        "the GUI stuff we use the datastructures returned by "
		        "the api directly. And If we output, it is stdout "
		        "everywhere.\n");

	ATF_CHECK(issue.labels_size == 0);
	ATF_CHECK(issue.labels == NULL);

	ATF_CHECK(issue.assignees_size == 0);
	ATF_CHECK(issue.assignees == NULL);

	ATF_CHECK(issue.is_pr == 0);
	ATF_CHECK(!issue.milestone);

	json_close(&stream);
	gcli_issue_free(&issue);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_pull);
ATF_TC_BODY(simple_github_pull, tc)
{
	struct gcli_pull pull = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_pull.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_pull(ctx, &stream, &pull) == 0);

	ATF_CHECK_STREQ(pull.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(pull.state, "closed");
	ATF_CHECK_STREQ(pull.title, "mark notifications as read/done");
	ATF_CHECK_STREQ(pull.body, "Fixes #99\n");
	ATF_CHECK_STREQ(pull.created_at, "2022-03-22T13:20:57Z");
	ATF_CHECK_STREQ(pull.head_label, "herrhotzenplotz:99");
	ATF_CHECK_STREQ(pull.base_label, "herrhotzenplotz:trunk");
	ATF_CHECK_STREQ(pull.head_sha, "a00f475af1e31d56c7a5839508a21e2b76a31e49");
	ATF_CHECK(pull.milestone == NULL);
	ATF_CHECK(pull.id == 886044243);
	ATF_CHECK(pull.comments == 0);
	ATF_CHECK(pull.additions == 177);
	ATF_CHECK(pull.deletions == 82);
	ATF_CHECK(pull.commits == 6);
	ATF_CHECK(pull.changed_files == 13);

	ATF_CHECK(pull.labels == NULL);
	ATF_CHECK(pull.labels_size == 0);
	ATF_CHECK(pull.merged == true);
	ATF_CHECK(pull.mergeable == false);
	ATF_CHECK(pull.draft == false);

	json_close(&stream);
	gcli_pull_free(&pull);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_label);
ATF_TC_BODY(simple_github_label, tc)
{
	struct gcli_label label = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_label.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_label(ctx, &stream, &label) == 0);

	ATF_CHECK(label.id == 3431203676);
	ATF_CHECK_STREQ(label.name, "bug");
	ATF_CHECK_STREQ(label.description, "Something isn't working");
	ATF_CHECK(label.colour == 0xd73a4a00);

	json_close(&stream);
	gcli_free_label(&label);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_milestone);
ATF_TC_BODY(simple_github_milestone, tc)
{
	struct gcli_milestone milestone = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_milestone.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_milestone(ctx, &stream, &milestone) == 0);

	ATF_CHECK(milestone.id == 1);
	ATF_CHECK_STREQ(milestone.title, "Gitlab support");
	ATF_CHECK_STREQ(milestone.state, "open");
	ATF_CHECK_STREQ(milestone.created_at, "2021-12-14T07:02:05Z");
	ATF_CHECK_STREQ(milestone.description, "");
	ATF_CHECK_STREQ(milestone.updated_at, "2021-12-19T14:49:43Z");
	ATF_CHECK(milestone.due_date == NULL);
	ATF_CHECK(milestone.expired == false);
	ATF_CHECK(milestone.open_issues == 0);
	ATF_CHECK(milestone.closed_issues == 8);

	json_close(&stream);
	gcli_free_milestone(&milestone);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_release);
ATF_TC_BODY(simple_github_release, tc)
{
	struct gcli_release release = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_release.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_release(ctx, &stream, &release) == 0);

	ATF_CHECK_STREQ(release.id, "116031718");
	ATF_CHECK(release.assets_size == 0);
	ATF_CHECK(release.assets == NULL);
	ATF_CHECK_STREQ(release.name, "1.2.0");
	ATF_CHECK_STREQ(release.body, "# Version 1.2.0\n\nThis is version 1.2.0 of gcli.\n\n## Notes\n\nPlease test and report bugs.\n\nYou can download autotoolized tarballs at: https://herrhotzenplotz.de/gcli/releases/gcli-1.2.0/\n\n## Bug Fixes\n\n- Fix compile error when providing --with-libcurl without any arguments\n- Fix memory leaks in string processing functions\n- Fix missing nul termination in read-file function\n- Fix segmentation fault when clearing the milestone of a PR on Gitea\n- Fix missing documentation for milestone action in issues and pulls\n- Set the 'merged' flag properly when showing Gitlab merge requests\n\n## New features\n\n- Add a config subcommand for managing ssh keys (see gcli-config(1))\n- Show number of comments/notes in list of issues and PRs\n- Add support for milestone management in pull requests\n");
	ATF_CHECK_STREQ(release.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(release.date, "2023-08-11T07:42:37Z");
	ATF_CHECK_STREQ(release.upload_url, "https://uploads.github.com/repos/herrhotzenplotz/gcli/releases/116031718/assets{?name,label}");
	ATF_CHECK(release.draft == false);
	ATF_CHECK(release.prerelease == false);

	json_close(&stream);
	gcli_release_free(&release);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_repo);
ATF_TC_BODY(simple_github_repo, tc)
{
	struct gcli_repo repo = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_repo.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_repo(ctx, &stream, &repo) == 0);

	ATF_CHECK(repo.id == 415015197);
	ATF_CHECK_STREQ(repo.full_name, "herrhotzenplotz/gcli");
	ATF_CHECK_STREQ(repo.name, "gcli");
	ATF_CHECK_STREQ(repo.owner, "herrhotzenplotz");
	ATF_CHECK_STREQ(repo.date, "2021-10-08T14:20:15Z");
	ATF_CHECK_STREQ(repo.visibility, "public");
	ATF_CHECK(repo.is_fork == false);

	json_close(&stream);
	gcli_repo_free(&repo);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_fork);
ATF_TC_BODY(simple_github_fork, tc)
{
	struct gcli_fork fork = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_fork.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_fork(ctx, &stream, &fork) == 0);

	ATF_CHECK_STREQ(fork.full_name, "gjnoonan/quick-lint-js");
	ATF_CHECK_STREQ(fork.owner, "gjnoonan");
	ATF_CHECK_STREQ(fork.date, "2023-05-11T05:37:41Z");
	ATF_CHECK(fork.forks == 0);

	json_close(&stream);
	gcli_fork_free(&fork);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_comment);
ATF_TC_BODY(simple_github_comment, tc)
{
	struct gcli_comment comment = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_comment.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_comment(ctx, &stream, &comment) == 0);

	ATF_CHECK(comment.id == 1424392601);
	ATF_CHECK_STREQ(comment.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(comment.date, "2023-02-09T15:37:54Z");
	ATF_CHECK_STREQ(comment.body, "Hey,\n\nthe current trunk on Github might be a little outdated. I pushed the staging branch for version 1.0.0 from Gitlab to Github (cleanup-1.0). Could you try again with that branch and see if it still faults at the same place? If it does, please provide a full backtrace and if possible check with valgrind.\n");

	json_close(&stream);
	gcli_comment_free(&comment);
	gcli_destroy(&ctx);
}

ATF_TC_WITHOUT_HEAD(simple_github_check);
ATF_TC_BODY(simple_github_check, tc)
{
    struct gcli_github_check check = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_check.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_check(ctx, &stream, &check) == 0);

	ATF_CHECK_STREQ(check.name, "test Windows x86");
	ATF_CHECK_STREQ(check.status, "completed");
	ATF_CHECK_STREQ(check.conclusion, "success");
	ATF_CHECK_STREQ(check.started_at, "2023-09-02T06:27:37Z");
	ATF_CHECK_STREQ(check.completed_at, "2023-09-02T06:29:11Z");
	ATF_CHECK(check.id == 16437184455);

	json_close(&stream);
	gcli_github_check_free(&check);
	gcli_destroy(&ctx);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_github_issue);
	ATF_TP_ADD_TC(tp, simple_github_pull);
	ATF_TP_ADD_TC(tp, simple_github_label);
	ATF_TP_ADD_TC(tp, simple_github_milestone);
	ATF_TP_ADD_TC(tp, simple_github_release);
	ATF_TP_ADD_TC(tp, simple_github_repo);
	ATF_TP_ADD_TC(tp, simple_github_fork);
	ATF_TP_ADD_TC(tp, simple_github_comment);
	ATF_TP_ADD_TC(tp, simple_github_check);

	return atf_no_error();
}
