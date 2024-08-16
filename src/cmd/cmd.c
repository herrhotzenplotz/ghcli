/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/repos.h>

#include <ctype.h>
#include <stdlib.h>

#include <curl/curl.h>

void
copyright(void)
{
	fprintf(
		stderr,
		"Copyright 2021, 2022, 2023 Nico Sonack <nsonack@herrhotzenplotz.de>"
		" and contributors.\n");
}

void
version(void)
{
	fprintf(stderr, PACKAGE_STRING" ("HOSTOS")\n");
}

void
longversion(void)
{
	version();
	fprintf(stderr,
	        "Using %s\n"
	        "Using vendored pdjson library\n"
	        "\n"
	        "Project website: "PACKAGE_URL"\n"
	        "Bug reports: "PACKAGE_BUGREPORT"\n",
	        curl_version());
}

void
check_owner_and_repo(const char **owner, const char **repo)
{
	/* HACK */
	if (gcli_config_get_forge_type(g_clictx) == GCLI_FORGE_BUGZILLA)
		return;

	/* If no remote was specified, try to autodetect */
	if ((*owner == NULL) != (*repo == NULL))
		errx(1, "gcli: error: missing either explicit owner or repo");

	if (*owner == NULL) {
		int rc = gcli_config_get_repo(g_clictx, owner, repo);
		if (rc < 0)
			errx(1, "gcli: error: %s", gcli_get_error(g_clictx));
	}
}

/* Parses (and updates) the given argument list into two seperate lists:
 *
 *   --add    -> add_labels
 *   --remove -> remove_labels
 */
void
parse_labels_options(int *argc, char ***argv,
                     const char ***_add_labels,    size_t *_add_labels_size,
                     const char ***_remove_labels, size_t *_remove_labels_size)
{
	const char **add_labels = NULL, **remove_labels = NULL;
	size_t       add_labels_size = 0, remove_labels_size = 0;

	/* Collect add/delete labels */
	while (*argc > 0) {
		if (strcmp(**argv, "add") == 0) {
			shift(argc, argv);

			add_labels = realloc(
				add_labels,
				(add_labels_size + 1) * sizeof(*add_labels));
			add_labels[add_labels_size++] = shift(argc, argv);
		} else if (strcmp(**argv, "remove") == 0) {
			shift(argc, argv);

			remove_labels = realloc(
				remove_labels,
				(remove_labels_size + 1) * sizeof(*remove_labels));
			remove_labels[remove_labels_size++] = shift(argc, argv);
		} else {
			break;
		}
	}

	*_add_labels      = add_labels;
	*_add_labels_size = add_labels_size;

	*_remove_labels      = remove_labels;
	*_remove_labels_size = remove_labels_size;
}

/* delete the repo (and ask for confirmation)
 *
 * NOTE: this procedure is here because it is used by both the forks
 * and repo subcommand. Ideally it should be moved into the 'repos'
 * code but I don't wanna make it exported from there. */
void
delete_repo(bool always_yes, const char *owner, const char *repo)
{
	bool delete = false;

	if (!always_yes) {
		delete = sn_yesno(
			"Are you sure you want to delete %s/%s?",
			owner, repo);
	} else {
		delete = true;
	}

	if (!delete)
		errx(1, "gcli: Operation aborted");

	if (gcli_repo_delete(g_clictx, owner, repo) < 0)
		errx(1, "gcli: error: failed to delete repo");
}

static int
word_length(const char *x)
{
	int l = 0;

	while (*x && !isspace(*x++))
		l++;
	return l;
}

void
gcli_pretty_print(const char *input, int indent, int maxlinelen, FILE *out)
{
	const char *it = input;

	if (!it)
		return;

	while (*it) {
		int linelength = indent;
		fprintf(out, "%*.*s", indent, indent, "");

		do {
			int w = word_length(it) + 1;

			if (it[w - 1] == '\n') {
				fprintf(out, "%.*s", w - 1, it);
				it += w;
				break;
			} else if (it[w - 1] == '\0') {
				w -= 1;
			}

			fprintf(out, "%.*s", w, it);
			it += w;
			linelength += w;

		} while (*it && (linelength < maxlinelen));

		fputc('\n', out);
	}
}
