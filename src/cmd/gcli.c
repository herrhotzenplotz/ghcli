/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_GETOPT_h
#include <getopt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gcli/cmd/attachments.h>
#include <gcli/cmd/ci.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/comment.h>
#include <gcli/cmd/config.h>
#include <gcli/cmd/forks.h>
#include <gcli/cmd/gists.h>
#include <gcli/cmd/issues.h>
#include <gcli/cmd/labels.h>
#include <gcli/cmd/milestones.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/pulls.h>
#include <gcli/cmd/releases.h>
#include <gcli/cmd/repos.h>
#include <gcli/cmd/snippets.h>
#include <gcli/cmd/status.h>

static void usage(void);

static int
subcommand_version(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	longversion();
	copyright();

	return EXIT_SUCCESS;
}

static struct subcommand {
	char const *cmd_name;
	char const *docstring;
	int (*fn)(int, char **);
} default_subcommands[] = {
	{ .cmd_name = "ci",
	  .fn = subcommand_ci,
	  .docstring = "Github CI status info" },
	{ .cmd_name = "comment",
	  .fn = subcommand_comment,
	  .docstring = "Comment under issues and PRs" },
	{ .cmd_name = "config",
	  .fn = subcommand_config,
	  .docstring = "Configure forges" },
	{ .cmd_name = "forks",
	  .fn = subcommand_forks,
	  .docstring = "Create, delete and list repository forks" },
	{ .cmd_name = "gists",
	  .fn = subcommand_gists,
	  .docstring = "Create, fetch and list Github Gists" },
	{ .cmd_name = "issues",
	  .fn = subcommand_issues,
	  .docstring = "Manage issues" },
	{ .cmd_name = "labels",
	  .fn = subcommand_labels,
	  .docstring = "Manage issue and PR labels" },
	{ .cmd_name = "milestones",
	  .fn = subcommand_milestones,
	  .docstring = "Milestone handling" },
	{ .cmd_name = "pipelines",
	  .fn = subcommand_pipelines,
	  .docstring = "Gitlab CI management" },
	{ .cmd_name = "pulls",
	  .fn = subcommand_pulls,
	  .docstring = "Create, view and manage PRs" },
	{ .cmd_name = "releases",
	  .fn = subcommand_releases,
	  .docstring = "Manage releases of repositories" },
	{ .cmd_name = "repos",
	  .fn = subcommand_repos,
	  .docstring = "Remote Repository management" },
	{ .cmd_name = "snippets",
	  .fn = subcommand_snippets,
	  .docstring = "Fetch and list Gitlab snippets" },
	{ .cmd_name = "status",
	  .fn = subcommand_status,
	  .docstring = "General user status and notifications" },
	{ .cmd_name = "attachments",
	  .fn = subcommand_attachments,
	  .docstring = "Bugzilla Attachments management" },
	{ .cmd_name = "api",
	  .fn = subcommand_api,
	  .docstring = "Fetch plain JSON info from an API (for debugging purposes)" },
	{ .cmd_name = "version",
	  .fn = subcommand_version,
	  .docstring = "Print version" },
};

static struct subcommand *subcommands = NULL;
static size_t subcommands_size = 0;

static void
usage(void)
{
	fprintf(stderr, "usage: gcli [options] subcommand\n\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -a account       Use the configured account instead of inferring it\n");
	fprintf(stderr, "  -r remote        Infer account from the given git remote\n");
	fprintf(stderr, "  -t type          Force the account type:\n");
	fprintf(stderr, "                      - github (default: github.com)\n");
	fprintf(stderr, "                      - gitlab (default: gitlab.com)\n");
	fprintf(stderr, "                      - gitea (default: codeberg.org)\n");
	fprintf(stderr, "                      - bugzilla (default: bugs.freebsd.org)\n");
	fprintf(stderr, "  -c               Force colour and text formatting.\n");
	fprintf(stderr, "  --no-spinner     Disable the animated spinner.\n");
	fprintf(stderr, "  -q               Be quiet. (Not implemented yet)\n");
	fprintf(stderr, "  -v               Be verbose.\n");
	fprintf(stderr, "  -V | --version   Print version and exit.\n\n");
	fprintf(stderr, "SUBCOMMANDS:\n");
	for (size_t i = 0; i < subcommands_size; ++i) {
		fprintf(stderr,
		        "  %-15.15s  %s\n",
		        subcommands[i].cmd_name,
		        subcommands[i].docstring);
	}
	fprintf(stderr, "\n");
	version();
	copyright();
}

/** The CMD global gcli context */
struct gcli_ctx *g_clictx = NULL;

static void
gcli_progress_func(bool const done)
{
	char spinner[] = "|/-\\";
	static size_t const spinner_elems = sizeof(spinner) / sizeof(*spinner);
	static int spinner_idx = 0;
	static int have_checked_stderr = 0, stderr_is_tty = 1;

	/* Check if stderr is a tty */
	if (!have_checked_stderr) {
		stderr_is_tty = isatty(STDERR_FILENO);
		have_checked_stderr = 1;
	}

	if (!stderr_is_tty)
		return;

	/* Clear out the line when done */
	if (done) {
		fprintf(stderr, "          \r");
	} else {
		fprintf(stderr, "Wait... %c\r", spinner[spinner_idx]);
		spinner_idx = (spinner_idx + 1) % (spinner_elems - 1);
	}
}

/* Abbreviated form matching:
 *
 *  - we presort the subcommands array alphabetised
 *  - then we can simply match by prefix */
static int
subcommand_compare(void const *s1, void const *s2)
{
	struct subcommand const *sc1 = s1;
	struct subcommand const *sc2 = s2;

	return strcmp(sc1->cmd_name, sc2->cmd_name);
}

static void
presort_subcommands(void)
{
	qsort(subcommands, subcommands_size, sizeof(*subcommands),
	      subcommand_compare);
}

static bool
is_unique_match(size_t const idx, char const *const name, size_t const name_len)
{
	/* Last match is always unique */
	if (idx + 1 == subcommands_size)
		return true;

	for (size_t i = idx + 1; i < subcommands_size; ++i) {
		if (strncmp(name, subcommands[i].cmd_name, name_len))
			return true; /* doesn't match. meaning this one is unique. */
		else
			break; /* we found a duplicate prefix. */
	}

	fprintf(stderr, "gcli: error: %s: subcommand is ambiguous. could be one of:\n", name);
	/* List until either the end or until we don't match any more prefixes */
	for (size_t i = idx; i < subcommands_size; ++i) {
		if (strncmp(name, subcommands[i].cmd_name, name_len))
			break;

		fprintf(stderr, "  - %-13.13s  %s\n", subcommands[i].cmd_name,
		        subcommands[i].docstring);
	}

	fprintf(stderr, "\n");

	return false;
}

enum {
	LOOKUP_NOSUCHCMD = 1,
	LOOKUP_AMBIGUOUS,
};

static struct subcommand const *
find_subcommand(char const *const name, int *error)
{
	size_t const name_len = strlen(name);

	for (size_t i = 0; i < subcommands_size; ++i) {
		if (strncmp(subcommands[i].cmd_name, name, name_len) == 0) {
			/* At least the prefix matches. Check that it is a unique match. */
			if (!is_unique_match(i, name, name_len)) {
				if (error)
					*error = LOOKUP_AMBIGUOUS;

				return NULL;
			}

			return subcommands + i;
		}
	}

	/* no match */
	fprintf(stderr, "gcli: error: %s: no such subcommand\n", name);
	if (error)
		*error = LOOKUP_NOSUCHCMD;

	return NULL;
}

static void
add_subcommand_alias(char const *alias_name, char const *alias_for)
{
	char *docstring;
	struct subcommand const *old_sc;
	struct subcommand *new_sc;
	int (*old_fn)(int, char **);

	old_sc = find_subcommand(alias_for, NULL);
	if (old_sc == NULL) {
		fprintf(stderr, "gcli: note: this error occured while defining the alias »%s«\n",
		        alias_name);
		exit(EXIT_FAILURE);
	}

	old_fn = old_sc->fn;
	docstring = sn_asprintf("Alias for %s", alias_for);
	subcommands = realloc(subcommands, (subcommands_size + 1) * sizeof(*subcommands));

	/* Copy in data */
	new_sc = &subcommands[subcommands_size++];

	new_sc->cmd_name = alias_name;
	new_sc->fn = old_fn;
	new_sc->docstring = docstring;
}

static void
install_aliases(void)
{
	struct gcli_config_entries const *entries;
	struct gcli_config_entry const *entry;

	add_subcommand_alias("pr", "pulls");

	/* Search for aliases in the user config file */
	entries = gcli_config_get_section_entries(g_clictx, "aliases");
	if (!entries)
		return;

	TAILQ_FOREACH(entry, entries, next) {
		char *alias_name, *alias_for;

		alias_name = sn_sv_to_cstr(entry->key);
		alias_for = sn_sv_to_cstr(entry->value);

		add_subcommand_alias(alias_name, alias_for);

		free(alias_for);
	}
}

static void
setup_subcommand_table(void)
{
	subcommands = calloc(sizeof(*subcommands), ARRAY_SIZE(default_subcommands));
	memcpy(subcommands, default_subcommands, sizeof(default_subcommands));
	subcommands_size = ARRAY_SIZE(default_subcommands);
}

int
main(int argc, char *argv[])
{
	char const *errmsg;
	struct subcommand const *sc;
	int error_reason;

	errmsg = gcli_init(&g_clictx, gcli_config_get_forge_type,
	                   gcli_config_get_token, gcli_config_get_apibase);
	if (errmsg)
		errx(1, "gcli: error: %s", errmsg);

	if (gcli_config_init_ctx(g_clictx) < 0)
		errx(1, "gcli: error: failed to init context: %s", gcli_get_error(g_clictx));

	/* Initial setup */
	setup_subcommand_table();

	/* Install subcommand aliases into subcommand table */
	install_aliases();

	/* Sorts the subcommands array alphabatically */
	presort_subcommands();

	/* Parse first arguments */
	if (gcli_config_parse_args(g_clictx, &argc, &argv)) {
		usage();
		return EXIT_FAILURE;
	}

	/* Make sure we have a subcommand */
	if (argc == 0) {
		fprintf(stderr, "gcli: error: missing subcommand\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Search for the subcommand */
	sc = find_subcommand(argv[0], &error_reason);
	if (sc == NULL) {
		if (error_reason == LOOKUP_AMBIGUOUS)
			version();
		else
			usage();

		return EXIT_FAILURE;
	}

	if(gcli_config_display_progress_spinner(g_clictx))
		gcli_set_progress_func(g_clictx, gcli_progress_func);

	return sc->fn(argc, argv);
}
