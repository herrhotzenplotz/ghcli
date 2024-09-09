/*
 * Copyright 2021-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <stdlib.h>

#include <gcli/forges.h>

#include <gcli/github/api.h>
#include <gcli/github/checkout.h>
#include <gcli/github/comments.h>
#include <gcli/github/config.h>
#include <gcli/github/forks.h>
#include <gcli/github/issues.h>
#include <gcli/github/labels.h>
#include <gcli/github/milestones.h>
#include <gcli/github/pulls.h>
#include <gcli/github/releases.h>
#include <gcli/github/repos.h>
#include <gcli/github/sshkeys.h>
#include <gcli/github/status.h>

#include <gcli/gitlab/api.h>
#include <gcli/gitlab/checkout.h>
#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/forks.h>
#include <gcli/gitlab/issues.h>
#include <gcli/gitlab/labels.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/milestones.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/gitlab/releases.h>
#include <gcli/gitlab/repos.h>
#include <gcli/gitlab/status.h>
#include <gcli/gitlab/sshkeys.h>

#include <gcli/gitea/comments.h>
#include <gcli/gitea/config.h>
#include <gcli/gitea/forks.h>
#include <gcli/gitea/issues.h>
#include <gcli/gitea/labels.h>
#include <gcli/gitea/milestones.h>
#include <gcli/gitea/pulls.h>
#include <gcli/gitea/releases.h>
#include <gcli/gitea/repos.h>
#include <gcli/gitea/sshkeys.h>
#include <gcli/gitea/status.h>

#include <gcli/bugzilla/api.h>
#include <gcli/bugzilla/attachments.h>
#include <gcli/bugzilla/bugs.h>
#include <gcli/bugzilla/config.h>

static struct gcli_forge_descriptor const
github_forge_descriptor =
{
	/* Comments */
	.get_comment               = github_get_comment,
	.get_issue_comments        = github_get_comments,
	.get_pull_comments         = github_get_comments,
	.perform_submit_comment    = github_perform_submit_comment,

	/* Forks */
	.fork_create               = github_fork_create,
	.get_forks                 = github_get_forks,

	/* Issues */
	.get_issue_summary         = github_get_issue_summary,
	.search_issues             = github_issues_search,
	.issue_add_labels          = github_issue_add_labels,
	.issue_assign              = github_issue_assign,
	.issue_clear_milestone     = github_issue_clear_milestone,
	.issue_close               = github_issue_close,
	.issue_remove_labels       = github_issue_remove_labels,
	.issue_reopen              = github_issue_reopen,
	.issue_set_milestone       = github_issue_set_milestone,
	.issue_set_title           = github_issue_set_title,
	.perform_submit_issue      = github_perform_submit_issue,
	.issue_quirks              = GCLI_ISSUE_QUIRKS_PROD_COMP
	                           | GCLI_ISSUE_QUIRKS_URL
	                           | GCLI_ISSUE_QUIRKS_ATTACHMENTS,

	/* Milestones */
	.create_milestone          = github_create_milestone,
	.delete_milestone          = github_delete_milestone,
	.get_milestone             = github_get_milestone,
	.get_milestone_issues      = github_milestone_get_issues,
	.get_milestones            = github_get_milestones,
	.milestone_set_duedate     = github_milestone_set_duedate,

	/* Pull requests */
	.get_pull                  = github_get_pull,
	.get_pull_checks           = github_pull_get_checks,
	.get_pull_commits          = github_get_pull_commits,
	.search_pulls              = github_search_pulls,
	.perform_submit_pull       = github_perform_submit_pull,
	.pull_add_reviewer         = github_pull_add_reviewer,
	.pull_close                = github_pull_close,
	.pull_get_diff             = github_pull_get_diff,
	.pull_get_patch            = github_pull_get_patch,
	.pull_merge                = github_pull_merge,
	.pull_reopen               = github_pull_reopen,
	.pull_set_title            = github_pull_set_title,
	.pull_checkout             = github_pull_checkout,

	/* HACK: Here we can use the same functions as with issues because
	 * PRs are the same as issues on Github and the functions have the
	 * same types/arguments */
	.pull_add_labels           = github_issue_add_labels,
	.pull_clear_milestone      = github_issue_clear_milestone,
	.pull_remove_labels        = github_issue_remove_labels,
	.pull_set_milestone        = github_issue_set_milestone,

	.pull_create_review        = github_pull_create_review,

	.create_release            = github_create_release,
	.delete_release            = github_delete_release,
	.get_releases              = github_get_releases,

	/* Labels */
	.create_label              = github_create_label,
	.delete_label              = github_delete_label,
	.get_labels                = github_get_labels,

	/* Repos */
	.get_repos                 = github_get_repos,
	.repo_create               = github_repo_create,
	.repo_delete               = github_repo_delete,
	.repo_set_visibility       = github_repo_set_visibility,

	/* SSH Key management */
	.add_sshkey                = github_add_sshkey,
	.delete_sshkey             = github_delete_sshkey,
	.get_sshkeys               = github_get_sshkeys,

	/* Notifications */
	.get_notifications         = github_get_notifications,
	.notification_mark_as_read = github_notification_mark_as_read,
	.notification_get_issue    = github_notification_get_issue,
	.notification_get_comments = github_notification_get_comments,

	/* Internal stuff */
	.get_api_error_string      = github_api_error_string,
	.make_authheader           = github_make_authheader,
	.user_object_key           = "login",

	/* Quirks */
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_EXPIRED
	                           | GCLI_MILESTONE_QUIRKS_DUEDATE
	                           | GCLI_MILESTONE_QUIRKS_PULLS,
	.pull_summary_quirks       = GCLI_PRS_QUIRK_COVERAGE
	                           | GCLI_PRS_QUIRK_AUTOMERGE,
};

static struct gcli_forge_descriptor const
gitlab_forge_descriptor =
{
	/* Comments */
	.get_comment               = gitlab_get_comment,
	.get_issue_comments        = gitlab_get_issue_comments,
	.get_pull_comments         = gitlab_get_mr_comments,
	.perform_submit_comment    = gitlab_perform_submit_comment,

	/* Forks */
	.fork_create               = gitlab_fork_create,
	.get_forks                 = gitlab_get_forks,

	/* Issues */
	.get_issue_summary         = gitlab_get_issue_summary,
	.search_issues             = gitlab_issues_search,
	.issue_add_labels          = gitlab_issue_add_labels,
	.issue_assign              = gitlab_issue_assign,
	.issue_clear_milestone     = gitlab_issue_clear_milestone,
	.issue_close               = gitlab_issue_close,
	.issue_remove_labels       = gitlab_issue_remove_labels,
	.issue_reopen              = gitlab_issue_reopen,
	.issue_set_milestone       = gitlab_issue_set_milestone,
	.issue_set_title           = gitlab_issue_set_title,
	.perform_submit_issue      = gitlab_perform_submit_issue,
	.issue_quirks              = GCLI_ISSUE_QUIRKS_PROD_COMP
	                           | GCLI_ISSUE_QUIRKS_URL
	                           | GCLI_ISSUE_QUIRKS_ATTACHMENTS,

	/* Milestones */
	.create_milestone          = gitlab_create_milestone,
	.delete_milestone          = gitlab_delete_milestone,
	.get_milestone             = gitlab_get_milestone,
	.get_milestone_issues      = gitlab_milestone_get_issues,
	.get_milestones            = gitlab_get_milestones,
	.milestone_set_duedate     = gitlab_milestone_set_duedate,

	/* Pull requests */
	.get_pull                  = gitlab_get_pull,
	.get_pull_checks           = (gcli_get_pull_checks_cb)gitlab_get_mr_pipelines,
	.get_pull_commits          = gitlab_get_pull_commits,
	.search_pulls              = gitlab_get_mrs,
	.perform_submit_pull       = gitlab_perform_submit_mr,
	.pull_add_labels           = gitlab_mr_add_labels,
	.pull_add_reviewer         = gitlab_mr_add_reviewer,
	.pull_clear_milestone      = gitlab_mr_clear_milestone,
	.pull_close                = gitlab_mr_close,
	.pull_create_review        = gitlab_mr_create_review,
	.pull_get_diff             = gitlab_mr_get_diff,
	.pull_get_patch            = gitlab_mr_get_patch,
	.pull_merge                = gitlab_mr_merge,
	.pull_remove_labels        = gitlab_mr_remove_labels,
	.pull_reopen               = gitlab_mr_reopen,
	.pull_set_milestone        = gitlab_mr_set_milestone,
	.pull_set_title            = gitlab_mr_set_title,
	.pull_checkout             = gitlab_mr_checkout,

	/* Releases */
	.create_release            = gitlab_create_release,
	.delete_release            = gitlab_delete_release,
	.get_releases              = gitlab_get_releases,

	/* Labels */
	.create_label              = gitlab_create_label,
	.delete_label              = gitlab_delete_label,
	.get_labels                = gitlab_get_labels,

	/* Repos */
	.get_repos                 = gitlab_get_repos,
	.repo_create               = gitlab_repo_create,
	.repo_delete               = gitlab_repo_delete,
	.repo_set_visibility       = gitlab_repo_set_visibility,

	/* SSH Key management */
	.add_sshkey                = gitlab_add_sshkey,
	.delete_sshkey             = gitlab_delete_sshkey,
	.get_sshkeys               = gitlab_get_sshkeys,

	/* Notifications */
	.get_notifications         = gitlab_get_notifications,
	.notification_get_issue    = gitlab_notification_get_issue,
	.notification_mark_as_read = gitlab_notification_mark_as_read,
	.notification_get_comments = gitlab_notification_get_comments,

	/* Internal stuff */
	.get_api_error_string      = gitlab_api_error_string,
	.make_authheader           = gitlab_make_authheader,
	.user_object_key           = "username",

	/* Quirks */
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_NISSUES,
	.pull_summary_quirks       = GCLI_PRS_QUIRK_ADDDEL
	                           | GCLI_PRS_QUIRK_COMMITS
	                           | GCLI_PRS_QUIRK_CHANGES
	                           | GCLI_PRS_QUIRK_MERGED,
};

static struct gcli_forge_descriptor const
gitea_forge_descriptor =
{
	/* Comments */
	.get_comment               = gitea_get_comment,
	.get_issue_comments        = gitea_get_comments,
	.get_pull_comments         = gitea_get_comments,
	.perform_submit_comment    = gitea_perform_submit_comment,

	/* Forks */
	.fork_create               = gitea_fork_create,
	.get_forks                 = gitea_get_forks,

	/* Issues */
	.get_issue_summary         = gitea_get_issue_summary,
	.search_issues             = gitea_issues_search,
	.issue_add_labels          = gitea_issue_add_labels,
	.issue_assign              = gitea_issue_assign,
	.issue_clear_milestone     = gitea_issue_clear_milestone,
	.issue_close               = gitea_issue_close,
	.issue_remove_labels       = gitea_issue_remove_labels,
	.issue_reopen              = gitea_issue_reopen,
	.issue_set_milestone       = gitea_issue_set_milestone,
	.issue_set_title           = gitea_issue_set_title,
	.perform_submit_issue      = gitea_submit_issue,
	.issue_quirks              = GCLI_ISSUE_QUIRKS_PROD_COMP
	                           | GCLI_ISSUE_QUIRKS_URL
	                           | GCLI_ISSUE_QUIRKS_ATTACHMENTS,

	/* Milestones */
	.create_milestone          = gitea_create_milestone,
	.delete_milestone          = gitea_delete_milestone,
	.get_milestone             = gitea_get_milestone,
	.get_milestone_issues      = gitea_milestone_get_issues,
	.get_milestones            = gitea_get_milestones,
	.milestone_set_duedate     = gitea_milestone_set_duedate,

	/* Pull requests */
	.get_pull                  = gitea_get_pull,
	.get_pull_checks           = gitea_pull_get_checks, /* stub, will always return an error */
	.get_pull_commits          = gitea_get_pull_commits,
	.search_pulls              = gitea_search_pulls,
	.perform_submit_pull       = gitea_pull_submit,
	.pull_add_labels           = gitea_issue_add_labels,
	.pull_add_reviewer         = gitea_pull_add_reviewer,
	.pull_clear_milestone      = gitea_pull_clear_milestone,
	.pull_close                = gitea_pull_close,
	.pull_get_diff             = gitea_pull_get_diff,
	.pull_get_patch            = gitea_pull_get_patch,
	.pull_merge                = gitea_pull_merge,
	.pull_remove_labels        = gitea_issue_remove_labels,
	.pull_reopen               = gitea_pull_reopen,
	.pull_set_milestone        = gitea_pull_set_milestone,
	.pull_set_title            = gitea_pull_set_title,

	/* Releases */
	.create_release            = gitea_create_release,
	.delete_release            = gitea_delete_release,
	.get_releases              = gitea_get_releases,

	/* Labels */
	.create_label              = gitea_create_label,
	.delete_label              = gitea_delete_label,
	.get_labels                = gitea_get_labels,

	/* Repos */
	.get_repos                 = gitea_get_repos,
	.repo_create               = gitea_repo_create,
	.repo_delete               = gitea_repo_delete,
	.repo_set_visibility       = gitea_repo_set_visibility,

	/* SSH Key management */
	.add_sshkey                = gitea_add_sshkey,
	.delete_sshkey             = gitea_delete_sshkey,
	.get_sshkeys               = gitea_get_sshkeys,

	/* Notifications */
	.get_notifications         = gitea_get_notifications,
	.notification_mark_as_read = gitea_notification_mark_as_read,

	/* Internal stuff */
	.make_authheader           = gitea_make_authheader,
	.get_api_error_string      = github_api_error_string,    /* hack! */
	.user_object_key           = "username",

	/* Quirks */
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_EXPIRED
	                           | GCLI_MILESTONE_QUIRKS_PULLS,
	.pull_summary_quirks       = GCLI_PRS_QUIRK_COMMITS
	                           | GCLI_PRS_QUIRK_ADDDEL
	                           | GCLI_PRS_QUIRK_AUTOMERGE
	                           | GCLI_PRS_QUIRK_DRAFT
	                           | GCLI_PRS_QUIRK_CHANGES
	                           | GCLI_PRS_QUIRK_COVERAGE,
};

static struct gcli_forge_descriptor const
bugzilla_forge_descriptor =
{
	/* Issues */
	.search_issues             = bugzilla_get_bugs,
	.get_issue_summary         = bugzilla_get_bug,
	.get_issue_comments        = bugzilla_bug_get_comments,
	.get_comment               = bugzilla_bug_get_comment,
	.get_issue_attachments     = bugzilla_bug_get_attachments,
	.perform_submit_issue      = bugzilla_bug_submit,
	.issue_quirks              = GCLI_ISSUE_QUIRKS_COMMENTS
	                           | GCLI_ISSUE_QUIRKS_LOCKED,

	.attachment_get_content    = bugzilla_attachment_get_content,

	/* Internal stuff */
	.make_authheader           = bugzilla_make_authheader,
	.get_api_error_string      = bugzilla_api_error_string,
	.user_object_key           = "---dummy---",
};

struct gcli_forge_descriptor const *
gcli_forge(struct gcli_ctx *ctx)
{
	switch (ctx->get_forge_type(ctx)) {
	case GCLI_FORGE_GITHUB:
		return &github_forge_descriptor;
	case GCLI_FORGE_GITLAB:
		return &gitlab_forge_descriptor;
	case GCLI_FORGE_GITEA:
		return &gitea_forge_descriptor;
	case GCLI_FORGE_BUGZILLA:
		return &bugzilla_forge_descriptor;
	default:
		errx(1,
		     "error: cannot determine forge type. try forcing an account "
		     "with -a, specifying -t or create a .gcli file.");
	}
	return NULL;
}
