/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <config.h>

#include <gcli/cmd/actions.h>
#include <gcli/cmd/ci.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/comment.h>
#include <gcli/cmd/editor.h>
#include <gcli/cmd/gitconfig.h>
#include <gcli/cmd/interactive.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/pulls.h>
#include <gcli/cmd/pull_reviews.h>
#include <gcli/cmd/table.h>

#include <gcli/comments.h>
#include <gcli/forges.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/pulls.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <assert.h>
#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pulls create [-o owner -r repo] [-f from]\n");
	fprintf(stderr, "                         [-t to] [-d] [-a] [-l label] [pull-request-title]\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] [-a] [-A author] [-n number]\n");
	fprintf(stderr, "                  [-L label] [-M milestone] [-s] [search-terms...]\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] -i pull-id actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -a              When listing PRs, show everything including closed and merged PRs.\n");
	fprintf(stderr, "                  When creating a PR enable automerge.\n");
	fprintf(stderr, "  -A author       Filter pull requests by the given author\n");
	fprintf(stderr, "  -L label        Filter pull requests by the given label\n");
	fprintf(stderr, "  -M milestone    Filter pull requests by the given milestone\n");
	fprintf(stderr, "  -d              Mark newly created PR as a draft\n");
	fprintf(stderr, "  -f owner:branch Specify the owner and branch of the fork that is the head of a PR.\n");
	fprintf(stderr, "  -l label        Add the given label when creating the PR\n");
	fprintf(stderr, "  -n number       Number of PRs to fetch (-1 = everything)\n");
	fprintf(stderr, "  -i id           ID of PR to perform actions on\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -t branch       Specify target branch of the PR\n");
	fprintf(stderr, "  -y              Do not ask for confirmation.\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  all                    Display status, commits, op and checks of the PR\n");
	fprintf(stderr, "  op                     Display original post\n");
	fprintf(stderr, "  status                 Display PR metadata\n");
	fprintf(stderr, "  comments               Display comments\n");
	fprintf(stderr, "  notes                  Alias for notes\n");
	fprintf(stderr, "  commits                Display commits of the PR\n");
	fprintf(stderr, "  ci                     Display CI/Pipeline status information about the PR\n");
	fprintf(stderr, "  merge [-s] [-D]        Merge the PR (-s = squash commits, -D = inhibit deleting source branch)\n");
	fprintf(stderr, "  milestone <id>         Assign this PR to a milestone\n");
	fprintf(stderr, "  milestone -d           Clear associated milestones from the PR\n");
	fprintf(stderr, "  close                  Close the PR\n");
	fprintf(stderr, "  reopen                 Reopen a closed PR\n");
	fprintf(stderr, "  labels ...             Add or remove labels:\n");
	fprintf(stderr, "                            add <name>\n");
	fprintf(stderr, "                            remove <name>\n");
	fprintf(stderr, "  diff                   Display changes as diff\n");
	fprintf(stderr, "  patch                  Display changes as patch series\n");
	fprintf(stderr, "  title <new-title>      Change the title of the pull request\n");
	fprintf(stderr, "  request-review <user>  Add <user> as a reviewer of the PR\n");
	fprintf(stderr, "  checkout               Do a git-checkout of this PR (GitHub- and GitLab only)\n");
	if (gcli_config_enable_experimental(g_clictx))
		fprintf(stderr, "  review                 Start a review of this PR\n");

	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gcli_print_pulls(enum gcli_output_flags const flags,
                 struct gcli_pull_list const *const list, int const max)
{
	int n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "NUMBER",  .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "MERGED",  .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0 },
		{ .name = "CREATOR", .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "NOTES",   .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->pulls_size == 0) {
		puts("No Pull Requests");
		return;
	}

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->pulls_size)
		n = list->pulls_size;
	else
		n = max;

	/* Fill the table */
	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: cannot init table");

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->pulls[n-i-1].number,
			                 list->pulls[n-i-1].state,
			                 list->pulls[n-i-1].merged,
			                 list->pulls[n-i-1].author,
			                 list->pulls[n-i-1].comments,
			                 list->pulls[n-i-1].title);
		}
	} else {
		for (int i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->pulls[i].number,
			                 list->pulls[i].state,
			                 list->pulls[i].merged,
			                 list->pulls[i].author,
			                 list->pulls[i].comments,
			                 list->pulls[i].title);
		}
	}

	gcli_tbl_end(table);
}

int
gcli_pull_print_diff(FILE *stream, struct gcli_path const *const path)
{
	return gcli_pull_get_diff(g_clictx, stream, path);
}

int
gcli_pull_print_patch(FILE *stream, struct gcli_path const *const path)
{
	return gcli_pull_get_patch(g_clictx, stream, path);
}

void
gcli_pull_print(struct gcli_pull const *const it)
{
	gcli_dict dict;
	struct gcli_forge_descriptor const *const forge = gcli_forge(g_clictx);
	int const quirks = forge->pull_summary_quirks;

	dict = gcli_dict_begin();

	gcli_dict_add(dict,           "NUMBER", 0, 0, "%"PRIid, it->number);
	gcli_dict_add_string(dict,    "TITLE", 0, 0, it->title);
	gcli_dict_add_string(dict,    "HEAD", 0, 0, it->head_label);
	gcli_dict_add_string(dict,    "BASE", 0, 0, it->base_label);
	gcli_dict_add_timestamp(dict, "CREATED", 0, 0, it->created_at);
	gcli_dict_add_string(dict,    "AUTHOR", GCLI_TBLCOL_BOLD, 0, it->author);
	gcli_dict_add_string(dict,    "STATE", GCLI_TBLCOL_STATECOLOURED, 0, it->state);
	gcli_dict_add(dict,           "COMMENTS", 0, 0, "%d", it->comments);

	if (it->milestone)
		gcli_dict_add_string(dict, "MILESTONE", 0, 0, it->milestone);

	if ((quirks & GCLI_PRS_QUIRK_ADDDEL) == 0)
		/* FIXME: move printing colours into the dictionary printer? */
		gcli_dict_add(dict, "ADD:DEL", 0, 0, "%s%d%s:%s%d%s",
		              gcli_setcolour(GCLI_COLOR_GREEN),
		              it->additions,
		              gcli_resetcolour(),
		              gcli_setcolour(GCLI_COLOR_RED),
		              it->deletions,
		              gcli_resetcolour());

	if ((quirks & GCLI_PRS_QUIRK_COMMITS) == 0)
		gcli_dict_add(dict, "COMMITS", 0, 0, "%d", it->commits);

	if ((quirks & GCLI_PRS_QUIRK_CHANGES) == 0)
		gcli_dict_add(dict, "CHANGED", 0, 0, "%d", it->changed_files);

	if ((quirks & GCLI_PRS_QUIRK_AUTOMERGE) == 0)
		gcli_dict_add_string(dict, "AUTOMERGE", 0, 0, sn_bool_yesno(it->automerge));

	if ((quirks & GCLI_PRS_QUIRK_MERGED) == 0)
		gcli_dict_add_string(dict, "MERGED", 0, 0, sn_bool_yesno(it->merged));

	gcli_dict_add_string(dict, "MERGEABLE", 0, 0, sn_bool_yesno(it->mergeable));
	if ((quirks & GCLI_PRS_QUIRK_DRAFT) == 0)
		gcli_dict_add_string(dict, "DRAFT", 0, 0, sn_bool_yesno(it->draft));

	if ((quirks & GCLI_PRS_QUIRK_COVERAGE) == 0 && it->coverage)
		gcli_dict_add_string(dict, "COVERAGE", 0, 0, it->coverage);

	if (it->labels_size) {
		gcli_dict_add_string_list(dict, "LABELS",
		                          (char const *const *)it->labels,
		                          it->labels_size);
	} else {
		gcli_dict_add_string(dict, "LABELS", 0, 0, "none");
	}

	if (it->reviewers_size) {
		gcli_dict_add_string_list(dict, "REVIEWERS",
		                          /* cast needed because of nested const */
		                          (char const *const *)it->reviewers,
		                          it->reviewers_size);
	} else {
		gcli_dict_add_string(dict, "REVIEWERS", 0, 0, "none");
	}

	gcli_dict_end(dict);
}

void
gcli_pull_print_op(struct gcli_pull const *const pull)
{
	if (pull->body)
		gcli_pretty_print(pull->body, 4, 80, stdout);
}

static void
gcli_print_checks_list(struct gcli_pull_checks_list const *const list)
{
	switch (list->forge_type) {
	case GCLI_FORGE_GITHUB:
		github_print_checks((struct github_check_list const *)(list));
		break;
	case GCLI_FORGE_GITLAB:
		gitlab_print_pipelines((struct gitlab_pipeline_list const*)(list));
		break;
	default:
		assert(0 && "unreachable");
	}
}

int
gcli_pull_checks(struct gcli_path const *const path)
{
	struct gcli_pull_checks_list list = {0};
	gcli_forge_type t = gcli_config_get_forge_type(g_clictx);

	list.forge_type = t;

	switch (t) {
	case GCLI_FORGE_GITHUB:
	case GCLI_FORGE_GITLAB: {
		int rc = gcli_pull_get_checks(g_clictx, path, &list);
		if (rc < 0)
			return rc;

		gcli_print_checks_list(&list);
		gcli_pull_checks_free(&list);

		return 0;
	} break;
	default:
		puts("No checks.");
		return 0;               /* no CI support / not implemented */
	}
}

/**
 * Get a copy of the first line of the passed string.
 */
static char *
cut_newline(char const *const _it)
{
	char *it = strdup(_it);
	char *foo = it;
	while (*foo) {
		if (*foo == '\n') {
			*foo = 0;
			break;
		}
		foo += 1;
	}

	return it;
}

void
gcli_print_commits(struct gcli_commit_list const *const list)
{
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "SHA",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_COLOUREXPL },
		{ .name = "AUTHOR",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "EMAIL",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DATE",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "MESSAGE", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->commits_size == 0) {
		puts("No commits");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not initialize table");

	for (size_t i = 0; i < list->commits_size; ++i) {
		char *message = cut_newline(list->commits[i].message);
		gcli_tbl_add_row(table, GCLI_COLOR_YELLOW, list->commits[i].sha,
		                 list->commits[i].author,
		                 list->commits[i].email,
		                 list->commits[i].date,
		                 message);
		free(message);          /* message is copied by the function above */
	}

	gcli_tbl_end(table);
}

int
gcli_pull_commits(struct gcli_path const *const path)
{
	struct gcli_commit_list commits = {0};
	int rc = 0;

	rc = gcli_pull_get_commits(g_clictx, path, &commits);
	if (rc < 0)
		return rc;

	gcli_print_commits(&commits);
	gcli_commits_free(&commits);

	return rc;
}

static void
pull_init_user_file(struct gcli_ctx *ctx, FILE *stream, void *_opts)
{
	struct gcli_submit_pull_options *opts = _opts;

	(void) ctx;
	fprintf(
		stream,
		"! PR TITLE : %s\n"
		"! Enter PR comments above.\n"
		"! All lines starting with '!' will be discarded.\n"
		"!\n"
		"! vim: ft=markdown\n",
		opts->title);
}

static char *
gcli_pull_get_user_message(struct gcli_submit_pull_options *opts)
{
	return gcli_editor_get_user_message(g_clictx, pull_init_user_file, opts);
}

/* Hack to retrieve the owner / name of the target repository.
 * We may have to change this thing in the future as it is kinda silly. */
static char const *
pull_request_target_owner(struct gcli_path const *const repo_path)
{
	assert(repo_path->kind == GCLI_PATH_DEFAULT);
	return repo_path->data.as_default.owner;
}

static char const *
pull_request_target_repo(struct gcli_path const *const repo_path)
{
	assert(repo_path->kind == GCLI_PATH_DEFAULT);
	return repo_path->data.as_default.repo;
}

static int
create_pull(struct gcli_submit_pull_options *const opts, int always_yes)
{
	opts->body = gcli_pull_get_user_message(opts);

	printf("The following PR will be created:\n"
	       "\n"
	       "TITLE   : %s\n"
	       "BASE    : %s\n"
	       "HEAD    : %s\n"
	       "IN      : %s/%s\n"
	       "MESSAGE :\n",
	       opts->title, opts->target_branch, opts->from,
	       pull_request_target_owner(&opts->target_repo),
	       pull_request_target_repo(&opts->target_repo));

	if (opts->body)
		gcli_pretty_print(opts->body, 4, 80, stdout);
	else
		puts("No message.");

	if (!always_yes) {
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "gcli: PR aborted.");
	}

	return gcli_pull_submit(g_clictx, opts);
}

static char const *
pr_try_derive_head(void)
{
	char const *account;
	sn_sv branch  = {0};

	if ((account = gcli_config_get_account_name(g_clictx)) == NULL) {
		errx(1,
		     "gcli: error: Cannot derive PR head. Please specify --from or set the"
		     " account in the users gcli config file.\n"
		     "gcli: note:  %s",
		     gcli_get_error(g_clictx));
	}

	if (!(branch = gcli_gitconfig_get_current_branch()).length) {
		errx(1,
		     "gcli: error: Cannot derive PR head. Please specify --from or, if you"
		     " are in »detached HEAD« state, checkout the branch you"
		     " want to pull request.");
	}

	return sn_asprintf("%s:"SV_FMT, account, SV_ARGS(branch));
}

static char *
derive_head(void)
{
	char const *account;
	sn_sv branch  = {0};

	if ((account = gcli_config_get_account_name(g_clictx)) == NULL)
		return NULL;

	branch = gcli_gitconfig_get_current_branch();
	if (branch.length == 0)
		return NULL;

	return sn_asprintf("%s:"SV_FMT, account, SV_ARGS(branch));
}

/** Interactive version of the create subcommand */
static int
subcommand_pull_create_interactive(struct gcli_submit_pull_options *const opts)
{
	char const *deflt_owner = NULL, *deflt_repo = NULL;
	int rc = 0;

	gcli_config_get_repo(g_clictx, &deflt_owner, &deflt_repo);

	/* PR Source */
	if (!opts->from) {
		char *tmp = NULL;

		tmp = derive_head();
		opts->from = gcli_cmd_prompt("From (owner:branch)", tmp);
		free(tmp);
		tmp = NULL;
	}

	/* PR Target */
	if (!opts->target_repo.data.as_default.owner)
		opts->target_repo.data.as_default.owner =
			gcli_cmd_prompt("Owner", deflt_owner);

	if (!opts->target_repo.data.as_default.repo)
		opts->target_repo.data.as_default.repo =
			gcli_cmd_prompt("Repository", deflt_repo);

	if (!opts->target_branch) {
		char *tmp = NULL;
		sn_sv base;

		base = gcli_config_get_base(g_clictx);
		if (base.length != 0)
			tmp = sn_sv_to_cstr(base);

		opts->target_branch = gcli_cmd_prompt("To Branch", tmp);

		free(tmp);
		tmp = NULL;
	}

	/* Meta */
	opts->title = gcli_cmd_prompt("Title", GCLI_PROMPT_RESULT_MANDATORY);
	opts->automerge = sn_yesno("Enable automerge?");

	/* Reviewers */
	for (;;) {
		char *response;

		response = gcli_cmd_prompt("Add reviewer? (name or leave empty)",
		                           GCLI_PROMPT_RESULT_OPTIONAL);
		if (response == NULL)
			break;

		opts->reviewers = realloc(
			opts->reviewers,
			(opts->reviewers_size + 1) * sizeof(*opts->reviewers));

		opts->reviewers[opts->reviewers_size++] = response;
	}

	/* create_pull is going to pop up the editor */
	rc = create_pull(opts, false);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: %s\n", gcli_get_error(g_clictx));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int
subcommand_pull_create(int argc, char *argv[])
{
	/* we'll use getopt_long here to parse the arguments */
	int ch;
	struct gcli_submit_pull_options opts   = {0};
	int always_yes = 0;

	const struct option options[] = {
		{ .name = "from",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'f' },
		{ .name = "to",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 't' },
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "draft",
		  .has_arg = no_argument,
		  .flag = &opts.draft,
		  .val = 1   },
		{ .name = "label",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'l' },
		{ .name = "automerge",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'a' },
		{ .name = "reviewer",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'R' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "ayf:t:do:r:l:R:", options, NULL)) != -1) {
		switch (ch) {
		case 'f':
			opts.from = optarg;
			break;
		case 't':
			opts.target_branch = optarg;
			break;
		case 'd':
			opts.draft = 1;
			break;
		case 'o':
			opts.target_repo.data.as_default.owner = optarg;
			break;
		case 'r':
			opts.target_repo.data.as_default.repo = optarg;
			break;
		case 'l': /* add a label */
			opts.labels = realloc(
				opts.labels, sizeof(*opts.labels) * (opts.labels_size + 1));
			opts.labels[opts.labels_size++] = optarg;
			break;
		case 'R': /* add a reviewer */
			opts.reviewers = realloc(
				opts.reviewers, sizeof(*opts.reviewers) * (opts.reviewers_size + 1));
			opts.reviewers[opts.reviewers_size++] = optarg;
			break;
		case 'y':
			always_yes = 1;
			break;
		case 'a':
			opts.automerge = true;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		return subcommand_pull_create_interactive(&opts);

	if (!opts.from)
		opts.from = pr_try_derive_head();

	if (!opts.target_branch) {
		sn_sv base = gcli_config_get_base(g_clictx);
		if (base.length == 0)
			errx(1,
			     "gcli: error: PR base is missing. Please either specify "
			     "--to branch-name or set pr.base in .gcli.");

		opts.target_branch = sn_sv_to_cstr(base);
	}

	check_path(&opts.target_repo);

	if (argc != 1) {
		fprintf(stderr, "gcli: error: Missing title to PR\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.title = argv[0];

	if (create_pull(&opts, always_yes) < 0)
		errx(1, "gcli: error: failed to submit pull request: %s",
		     gcli_get_error(g_clictx));

	free(opts.labels);

	return EXIT_SUCCESS;
}

/* Forward declaration */
static int handle_pull_actions(int argc, char *argv[],
                               struct gcli_path const *const path);

int
subcommand_pulls(int argc, char *argv[])
{
	char *endptr = NULL;
	enum gcli_output_flags flags = 0;
	int ch = 0;
	int n = 30;                 /* how many prs to fetch at least */
	struct gcli_path pull = {0};
	struct gcli_pull_fetch_details details = {0};
	struct gcli_pull_list pulls = {0};

	/* detect whether we wanna create a PR */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_pull_create(argc, argv);
	}

	struct option const options[] = {
		{ .name    = "all",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'a' },
		{ .name    = "author",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'A' },
		{ .name    = "label",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'L' },
		{ .name    = "milestone",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'M' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "id",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{0},
	};

	/* Parse commandline options */
	while ((ch = getopt_long(argc, argv, "+n:o:r:i:asA:L:M:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			pull.data.as_default.owner = optarg;
			break;
		case 'r':
			pull.data.as_default.repo = optarg;
			break;
		case 'i': {
			pull.data.as_default.id = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse pr number »%s«", optarg);

			if (pull.data.as_default.id == 0)
				errx(1, "gcli: error: pr number is out of range");
		} break;
		case 'n': {
			n = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse pr count »%s«", optarg);

			if (n < -1)
				errx(1, "gcli: error: pr count is out of range");

			if (n == 0)
				errx(1, "gcli: error: pr count must not be zero");
		} break;
		case 'a': {
			details.all = true;
		} break;
		case 'A': {
			details.author = optarg;
		} break;
		case 'L': {
			details.label = optarg;
		} break;
		case 'M': {
			details.milestone = optarg;
		} break;
		case 's': {
			flags |= OUTPUT_SORTED;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_path(&pull);

	/* In case no explicit PR number was specified, list all
	 * open PRs and exit */
	if (pull.data.as_default.id == 0) {
		char *search_term = NULL;

		/* Trailing arguments indicate a search term */
		if (argc)
			search_term = sn_join_with((char const *const *)argv, argc, " ");

		details.search_term = search_term;

		if (gcli_search_pulls(g_clictx, &pull, &details, n, &pulls) < 0)
			errx(1, "gcli: error: could not fetch pull requests: %s",
			     gcli_get_error(g_clictx));

		gcli_print_pulls(flags, &pulls, n);
		gcli_pulls_free(&pulls);

		free(search_term);
		details.search_term = search_term = NULL;

		return EXIT_SUCCESS;
	}

	/* If a PR number was given, require -a to be unset */
	if (details.all || details.author) {
		fprintf(stderr, "gcli: error: -a and -A cannot be combined with operations on a PR\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Hand off to actions handling */
	return handle_pull_actions(argc, argv, &pull);
}

static int
action_all(struct gcli_path const *path, struct gcli_pull *pull, int *argc,
           char **argv[])
{
	(void) argc;
	(void) argv;

	/* Print meta */
	gcli_pull_print(pull);

	/* OP */
	puts("\nORIGINAL POST");
	gcli_pull_print_op(pull);

	/* Commits */
	puts("\nCOMMITS");
	if (gcli_pull_commits(path)) {
		fprintf(stderr, "gcli: error: failed to fetch pull request checks: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	/* Checks */
	puts("\nCHECKS");
	if (gcli_pull_checks(path)) {
		fprintf(stderr, "gcli: error: failed to fetch pull request checks: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_op(struct gcli_path const *const path, struct gcli_pull *pull, int *argc,
          char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_pull_print_op(pull);

	return GCLI_EX_OK;
}

static int
action_status(struct gcli_path const *const path, struct gcli_pull *pull,
              int *argc, char **argv[])
{
	(void) path;
	(void) argc;
	(void) argv;

	gcli_pull_print(pull);

	return GCLI_EX_OK;
}

static int
action_commits(struct gcli_path const *const path, struct gcli_pull *pull,
               int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	int const rc = gcli_pull_commits(path);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to fetch commits: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_diff(struct gcli_path const *const path, struct gcli_pull *pull,
            int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_print_diff(stdout, path) < 0) {
		fprintf(stderr, "gcli: error: failed to fetch diff: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_patch(struct gcli_path const *const path, struct gcli_pull *pull,
             int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_print_patch(stdout, path) < 0) {
		fprintf(stderr, "gcli: error: failed to fetch patch: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

/* aliased to notes */
static int
action_comments(struct gcli_path const *const path, struct gcli_pull *pull,
                int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_comments(path) < 0) {
		fprintf(stderr, "gcli: error: failed to fetch pull comments: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_ci(struct gcli_path const *const path, struct gcli_pull *pull,
          int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_checks(path) < 0) {
		fprintf(stderr, "gcli: error: failed to fetch pull request checks: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_merge(struct gcli_path const *const path, struct gcli_pull *pull,
             int *argc, char **argv[])
{
	enum gcli_merge_flags flags = GCLI_PULL_MERGE_DELETEHEAD;

	(void) pull;

	/* Default behaviour */
	if (gcli_config_pr_inhibit_delete_source_branch(g_clictx))
	    flags = 0;

	if (*argc > 1) {
		/* Check whether the user intends a squash-merge
		 * and/or wants to delete the source branch of the
		 * PR */
		char const *word = (*argv)[1];
		if (strcmp(word, "-s") == 0 || strcmp(word, "--squash") == 0) {
			*argc -= 1;
			*argv += 1;

			flags |= GCLI_PULL_MERGE_SQUASH;
		} else if (strcmp(word, "-D") == 0 || strcmp(word, "--inhibit-delete") == 0) {
			*argc -= 1;
			*argv += 1;

			flags &= ~GCLI_PULL_MERGE_DELETEHEAD;
		}
	}

	if (gcli_pull_merge(g_clictx, path, flags) < 0) {
		fprintf(stderr, "gcli: error: failed to merge pull request: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_close(struct gcli_path const *const path, struct gcli_pull *pull,
             int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_close(g_clictx, path) < 0) {
		fprintf(stderr, "gcli: error: failed to close pull request: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_reopen(struct gcli_path const *const path, struct gcli_pull *pull,
              int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_pull_reopen(g_clictx, path) < 0) {
		fprintf(stderr, "gcli: error: failed to reopen pull request: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_labels(struct gcli_path const *const path, struct gcli_pull *pull,
              int *argc, char **argv[])
{
	const char **add_labels = NULL;
	size_t add_labels_size = 0;
	const char **remove_labels = NULL;
	size_t remove_labels_size = 0;
	int rc = 0;

	(void) pull;

	if (*argc == 0) {
		fprintf(stderr, "gcli: error: expected label action\n");
		return GCLI_EX_USAGE;
	}

	parse_labels_options(argc, argv, &add_labels, &add_labels_size,
	                     &remove_labels, &remove_labels_size);

	/* actually go about deleting and adding the labels */
	if (add_labels_size) {
		rc = gcli_pull_add_labels(g_clictx, path, add_labels, add_labels_size);

		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to add labels: %s\n",
			        gcli_get_error(g_clictx));

			rc = GCLI_EX_DATAERR;
			goto bail;
		}
	}

	if (remove_labels_size) {
		rc = gcli_pull_remove_labels(g_clictx, path, remove_labels, remove_labels_size);

		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to remove labels: %s\n",
			        gcli_get_error(g_clictx));

			rc = GCLI_EX_DATAERR;
			goto bail;
		}
	}

bail:
	free(add_labels);
	free(remove_labels);

	return rc;
}

static int
action_milestone(struct gcli_path const *const path, struct gcli_pull *pull,
                 int *argc, char **argv[])
{
	char const *arg;

	(void) pull;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing arguments to milestone action\n");
		return GCLI_EX_USAGE;
	}

	*argc -= 1;
	*argv += 1;
	arg = **argv;

	if (strcmp(arg, "-d") == 0) {
		if (gcli_pull_clear_milestone(g_clictx, path) < 0) {
			fprintf(stderr, "gcli: error: failed to clear milestone: %s\n",
			        gcli_get_error(g_clictx));

			return GCLI_EX_DATAERR;
		}

	} else {
		int milestone_id = 0;
		char *endptr;
		int rc = 0;

		milestone_id = strtoul(arg, &endptr, 10);
		if (endptr != arg + strlen(arg)) {
			fprintf(stderr, "gcli: error: cannot parse milestone id »%s«\n",
			        arg);

			return GCLI_EX_DATAERR;
		}

		rc = gcli_pull_set_milestone(g_clictx, path, milestone_id);
		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to set milestone: %s\n",
			        gcli_get_error(g_clictx));

			return GCLI_EX_DATAERR;
		}
	}

	return GCLI_EX_OK;
}

static int
action_request_review(struct gcli_path const *const path,
                      struct gcli_pull *pull, int *argc, char **argv[])
{
	char const *reviewer;
	int rc;

	(void) pull;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing user name for reviewer\n");
		return GCLI_EX_USAGE;
	}

	*argc -= 1;
	*argv += 1;

	reviewer = **argv;

	rc = gcli_pull_add_reviewer(g_clictx, path, reviewer);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: failed to request review: %s\n",
		        gcli_get_error(g_clictx));

		return GCLI_EX_DATAERR;
	}

	return GCLI_EX_OK;
}

static int
action_title(struct gcli_path const *const path,
             struct gcli_pull *pull, int *argc, char **argv[])
{
	int rc = 0;

	(void) pull;

	if (*argc < 2) {
		fprintf(stderr, "gcli: error: missing title\n");

		return GCLI_EX_USAGE;
	}

	rc = gcli_pull_set_title(g_clictx, path, (*argv)[1]);
	if (rc < 0) {
		errx(1, "gcli: error: failed to update review title: %s",
		     gcli_get_error(g_clictx));
	}

	*argc -= 1;
	*argv += 1;

	return GCLI_EX_OK;
}

static int
action_review(struct gcli_path const *const path, struct gcli_pull *pull,
              int *argc, char **argv[])
{
	(void) pull;
	(void) argc;
	(void) argv;

	if (gcli_config_enable_experimental(g_clictx) == false) {
		fprintf(
			stderr,
			"gcli: error: review is not available because it is "
			"considered experimental. To enable this feature set "
			"enable-experimental in your gcli config file or "
			"set GCLI_ENABLE_EXPERIMENTAL in your environment.\n"
		);

		return GCLI_EX_DATAERR;
	}

	do_review_session(path);

	return GCLI_EX_OK;
}

static int
action_checkout(struct gcli_path const *const path, struct gcli_pull *pull,
                int *argc, char **argv[])
{
	char *remote;
	int rc = 0;

	(void) pull;
	(void) argc;
	(void) argv;

	rc = gcli_config_get_remote(g_clictx, &remote);
	if (rc < 0) {
		fprintf(stderr, "gcli: error: %s\n", gcli_get_error(g_clictx));
		return GCLI_EX_DATAERR;
	}

	if (gcli_pull_checkout(g_clictx, remote, path) < 0) {
		fprintf(
			stderr,
			"gcli: error: failed to checkout pull: %s\n",
			gcli_get_error(g_clictx)
		);

		return GCLI_EX_DATAERR;
	}

	free(remote);

	*argc -= 1;
	*argv += 1;

	return GCLI_EX_OK;
}

struct gcli_cmd_actions pull_actions = {
	.fetch_item = (gcli_cmd_action_fetcher)gcli_get_pull,
	.free_item = (gcli_cmd_action_freeer)gcli_pull_free,
	.item_size = sizeof(struct gcli_pull),

	.defs = {
		{
			.name = "all",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler) action_all,
		},
		{
			.name = "op",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler) action_op,
		},
		{
			.name = "status",
			.needs_item = true,
			.handler = (gcli_cmd_action_handler) action_status,
		},
		{
			.name = "commits",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_commits,
		},
		{
			.name = "diff",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_diff,
		},
		{
			.name = "patch",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_patch,
		},
		{
			.name = "notes",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_comments,
		},
		{
			.name = "comments",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_comments,
		},
		{
			.name = "ci",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_ci,
		},
		{
			.name = "merge",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_merge,
		},
		{
			.name = "close",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_close,
		},
		{
			.name = "reopen",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_reopen,
		},
		{
			.name = "labels",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_labels,
		},
		{
			.name = "milestone",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_milestone,
		},
		{
			.name = "request-review",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_request_review,
		},
		{
			.name = "title",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_title,
		},
		{
			.name = "review",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_review,
		},
		{
			.name = "checkout",
			.needs_item = false,
			.handler = (gcli_cmd_action_handler) action_checkout,
		},
	},
};

/** Handling routine for Pull Request related actions specified on the
 * command line. Make sure that the usage at the top is consistent
 * with the actions implemented here. */
static int
handle_pull_actions(int argc, char *argv[], struct gcli_path const *const path)
{
	int const rc = gcli_cmd_actions_handle(&pull_actions, path, &argc, &argv);

	if (rc == GCLI_EX_USAGE)
		usage();

	if (rc)
		return 1;

	return 0;
}
