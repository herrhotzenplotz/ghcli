/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef FORGES_H
#define FORGES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/comments.h>
#include <gcli/curl.h>
#include <gcli/forks.h>
#include <gcli/issues.h>
#include <gcli/labels.h>
#include <gcli/milestones.h>
#include <gcli/pulls.h>
#include <gcli/releases.h>
#include <gcli/repos.h>
#include <gcli/sshkeys.h>
#include <gcli/status.h>

/* Hopefully temporary hack */
typedef int (*gcli_get_pull_checks_cb)(
	struct gcli_ctx *, char const *, char const *, gcli_id,
	struct gcli_pull_checks_list *);

/**
 * Struct of function pointers to perform actions in the given
 * forge. It is like a plugin system to dispatch. */
struct gcli_forge_descriptor {
	/**
	 * Submit a comment to a pull/mr or issue */
	int (*perform_submit_comment)(
		struct gcli_ctx *ctx,
		struct gcli_submit_comment_opts opts);

	/**
	 * List comments on the given issue */
	int (*get_issue_comments)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		struct gcli_comment_list *out);

	/**
	 * List comments on the given PR */
	int (*get_pull_comments)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pr,
		struct gcli_comment_list *out);

	/**
	 * List forks of the given repo */
	int (*get_forks)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		struct gcli_fork_list *out);

	/**
	 * Fork the given repo into the owner _in */
	int (*fork_create)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *_in);

	/**
	 * Get a list of issues on the given repo */
	int (*search_issues)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		struct gcli_issue_fetch_details const *details,
		int max,
		struct gcli_issue_list *out);

	/**
	 * Get a summary of an issue */
	int (*get_issue_summary)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue_number,
		struct gcli_issue *out);

	/**
	 * Close the given issue */
	int (*issue_close)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue_number);

	/**
	 * Reopen the given issue */
	int (*issue_reopen)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue_number);

	/**
	 * Assign an issue to a user */
	int (*issue_assign)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue_number,
		char const *assignee);

	/**
	 * Add labels to issues */
	int (*issue_add_labels)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from issues */
	int (*issue_remove_labels)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Submit an issue */
	int (*perform_submit_issue)(
		struct gcli_ctx *ctx,
		struct gcli_submit_issue_options *opts,
		struct gcli_issue *out);

	/**
	 * Change the title of an issue */
	int (*issue_set_title)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		char const *new_title);

	/**
	 * Get attachments of an issue */
	int (*get_issue_attachments)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		struct gcli_attachment_list *out);

	/**
	 * Dump the contents of the attachment to the given file */
	int (*attachment_get_content)(
		struct gcli_ctx *ctx,
		gcli_id id,
		FILE *out);

	/* Issue quirk bitmask */
	enum {
		GCLI_ISSUE_QUIRKS_LOCKED      = 0x1,
		GCLI_ISSUE_QUIRKS_COMMENTS    = 0x2,
		GCLI_ISSUE_QUIRKS_PROD_COMP   = 0x4,
		GCLI_ISSUE_QUIRKS_URL         = 0x8,
		GCLI_ISSUE_QUIRKS_ATTACHMENTS = 0x10,
	} const issue_quirks;

	/**
	 * Bitmask of exceptions/fields that the forge doesn't support */
	enum {
		GCLI_MILESTONE_QUIRKS_EXPIRED = 0x1,
		GCLI_MILESTONE_QUIRKS_DUEDATE = 0x2,
		GCLI_MILESTONE_QUIRKS_PULLS   = 0x4,
		GCLI_MILESTONE_QUIRKS_NISSUES = 0x8,
	} const milestone_quirks;

	/**
	 * Get list of milestones */
	int (*get_milestones)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		struct gcli_milestone_list *out);

	/**
	 * Get a single milestone */
	int (*get_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id milestone,
		struct gcli_milestone *out);

	/**
	 * create a milestone */
	int (*create_milestone)(
		struct gcli_ctx *ctx,
		struct gcli_milestone_create_args const *args);

	/**
	 * delete a milestone */
	int (*delete_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id milestone);

	/**
	 * delete a milestone */
	int (*milestone_set_duedate)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id milestone,
		char const *date);

	/**
	 * Get list of issues attached to this milestone */
	int (*get_milestone_issues)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id milestone,
		struct gcli_issue_list *out);

	/** Assign an issue to a milestone */
	int (*issue_set_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue,
		gcli_id milestone);

	/**
	 * Clear the milestones of an issue */
	int (*issue_clear_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id issue);

	/**
	 * Get a list of PRs/MRs on the given repo */
	int (*search_pulls)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		struct gcli_pull_fetch_details const *details,
		int max,
		struct gcli_pull_list *out);

	/**
	 * Fetch the PR diff into the file */
	int (*pull_get_diff)(
		struct gcli_ctx *ctx,
		FILE *stream,
		char const *owner,
		char const *reponame,
		gcli_id pr_number);

	/**
	 * Fetch the PR patch series into the file */
	int (*pull_get_patch)(
		struct gcli_ctx *ctx,
		FILE *stream,
		char const *owner,
		char const *repo,
		gcli_id pull_id);

	/**
	 * Return a list of checks associated with the given pull.
	 *
	 * The type of the returned list depends on the forge type. See
	 * the definition of struct gcli_pull_checks_list. */
	gcli_get_pull_checks_cb get_pull_checks;

	/**
	 * Merge the given PR/MR */
	int (*pull_merge)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		gcli_id pr_number,
		enum gcli_merge_flags flags);

	/**
	 * Reopen the given PR/MR */
	int (*pull_reopen)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		gcli_id pr_number);

	/**
	 * Close the given PR/MR */
	int (*pull_close)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		gcli_id pr_number);

	/**
	 * Submit PR/MR */
	int (*perform_submit_pull)(
		struct gcli_ctx *ctx,
		struct gcli_submit_pull_options *opts);

	/**
	 * Get a list of commits in the given PR/MR */
	int (*get_pull_commits)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pr_number,
		struct gcli_commit_list *out);

	/** Bitmask of unsupported fields in the pull summary for this
	 * forge */
	enum gcli_pull_summary_quirks {
		GCLI_PRS_QUIRK_ADDDEL     = 0x01,
		GCLI_PRS_QUIRK_COMMITS    = 0x02,
		GCLI_PRS_QUIRK_CHANGES    = 0x04,
		GCLI_PRS_QUIRK_MERGED     = 0x08,
		GCLI_PRS_QUIRK_DRAFT      = 0x10,
		GCLI_PRS_QUIRK_COVERAGE   = 0x20,
		GCLI_PRS_QUIRK_AUTOMERGE  = 0x40,
	} pull_summary_quirks;

	/**
	 * Get a summary of the given PR/MR */
	int (*get_pull)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pr_number,
		struct gcli_pull *out);

	/**
	 * Add labels to Pull Requests */
	int (*pull_add_labels)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from Pull Requests */
	int (*pull_remove_labels)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Assign a PR to a milestone */
	int (*pull_set_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pull,
		gcli_id milestone_id);

	/**
	 * Clear a milestone on a PR */
	int (*pull_clear_milestone)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pull);

	/**
     * Request review of a given pull request by a user */
	int (*pull_add_reviewer)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pull,
		char const *username);

	/**
	 * Change the title of a pull request */
	int (*pull_set_title)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_id pull,
		char const *new_title);

	/**
	 * Create a review on a PR */
	int (*pull_create_review)(
		struct gcli_ctx *ctx,
		struct gcli_pull_create_review_details const *details);

	/**
	 * Get a list of releases in the given repo */
	int (*get_releases)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		struct gcli_release_list *out);

	/**
	 * Create a new release */
	int (*create_release)(
		struct gcli_ctx *ctx,
		struct gcli_new_release const *release);

	/**
	 * Delete the release */
	int (*delete_release)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *id);

	/**
	 * Get a list of labels that are valid in the given repository */
    int (*get_labels)(
	    struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		struct gcli_label_list *out);

	/**
	 * Create the given label
	 *
	 * The ID will be filled in for you */
	int (*create_label)(
	    struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		struct gcli_label *label);

	/**
	 * Delete the given label */
	int (*delete_label)(
	    struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *label);

	/**
	 * Get a list of repos of the given owner */
	int (*get_repos)(
		struct gcli_ctx *ctx,
		char const *owner,
		int max,
		struct gcli_repo_list *out);

	/**
	 * Create the given repo */
	int (*repo_create)(
		struct gcli_ctx *ctx,
		struct gcli_repo_create_options const *options,
		struct gcli_repo *out);

	/**
	 * Delete the given repo */
	int (*repo_delete)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo);

	/**
	 * Change the visibility level of a repository */
	int (*repo_set_visibility)(
		struct gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_repo_visibility vis);

	/**
	 * Status summary for the account */
	int (*get_notifications)(
		struct gcli_ctx *ctx,
		int max,
		struct gcli_notification_list *notifications);

	/**
	 * *Given an issue notification grab the associated issue. */
	int (*notification_get_issue)(
		struct gcli_ctx *ctx,
		struct gcli_notification const *const notification,
		struct gcli_issue *out);

	/**
	 * Given a notification fetch the target's comments */
	int (*notification_get_comments)(
		struct gcli_ctx *ctx,
		struct gcli_notification const *notification,
		struct gcli_comment_list *out);

	/**
	 * Mark notification with the given id as read
	 *
	 * Returns 0 on success or negative code on failure. */
	int (*notification_mark_as_read)(
		struct gcli_ctx *ctx,
		char const *id);

	/**
	 * Get an the http authentication header for use by curl */
	char *(*make_authheader)(struct gcli_ctx *ctx, char const *token);

	/**
	 * Get list of SSH keys */
	int (*get_sshkeys)(struct gcli_ctx *ctx, struct gcli_sshkey_list *);

	/**
	 * Add an SSH public key */
	int (*add_sshkey)(
		struct gcli_ctx *ctx,
		char const *title,
		char const *public_key_path,
		struct gcli_sshkey *out);

	/**
	 * Delete an SSH public key by its ID */
	int (*delete_sshkey)(struct gcli_ctx *ctx, gcli_id id);

	/**
	 * Get the error string from the API */
	char const *(*get_api_error_string)(
		struct gcli_ctx *ctx,
		struct gcli_fetch_buffer *);

	/**
	 * A key in the user json object sent by the API that represents
	 * the user name */
	char const *user_object_key;
};

struct gcli_forge_descriptor const *gcli_forge(struct gcli_ctx *ctx);

/** A macro used for calling one of the dispatch points above.
 *
 * It check whether the given function pointer is null. If it is it will return
 * an error message otherwise the function is called with the specified
 * arguments. */
#define gcli_null_check_call(routine, ctx, ...)                                 \
	do {                                                                        \
		struct gcli_forge_descriptor const *const forge = gcli_forge(ctx);      \
		                                                                        \
		if (forge->routine) {                                                   \
			return forge->routine(ctx, __VA_ARGS__);                            \
		} else {                                                                \
			return gcli_error(ctx, #routine " is not available on this forge"); \
		}                                                                       \
	} while (0)


#endif /* FORGES_H */
