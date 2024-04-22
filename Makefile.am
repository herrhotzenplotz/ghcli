ACLOCAL_AMFLAGS = -I m4

AM_YFLAGS = -d

AM_CPPFLAGS = -I$(srcdir)/include
AM_CPPFLAGS += -I$(srcdir)/thirdparty
AM_CPPFLAGS += -I$(srcdir)
AM_CPPFLAGS += -DYY_NO_INPUT
AM_CPPFLAGS += -DYY_NO_UNPUT
AM_CPPFLAGS += -D_XOPEN_SOURCE=600
AM_CFLAGS = -Wno-gnu-zero-variadic-macro-arguments

noinst_PROGRAMS = pgen$(EXEEXT)
pgen_SOURCES = \
	include/gcli/pgen.h \
	thirdparty/sn/sn.c \
	src/pgen/dump_c.c \
	src/pgen/dump_h.c \
	src/pgen/dump_plain.c \
	src/pgen/parser.y \
	src/pgen/lexer.l

lib_LTLIBRARIES = libgcli.la
noinst_LTLIBRARIES = libpdjson.la

# For testing puproses I'll reenable parallel builds. If it breaks again, uncomment.
# .NOTPARALLEL: pgen$(EXEEXT) $(builddir)/src/pgen/parser.c $(builddir)/src/pgen/parser.h $(builddir)/src/pgen/lexer.c
src/pgen/lexer.c: src/pgen/parser.h

libgcli_la_DEPENDENCIES = pgen$(EXEEXT) libpdjson.la

$(BUILT_SOURCES): pgen$(EXEEXT)

SUFFIXES = .t
.t.h:
	$(AM_V_GEN)./pgen -th -o $@ $<

.t.c:
	$(AM_V_GEN)./pgen -tc -o $@ $<

bin_PROGRAMS = gcli$(EXEEXT)
gcli_LDADD = libgcli.la

libgcli_la_LIBADD = libpdjson.la
libgcli_la_LDFLAGS = -no-undefined

dist_man_MANS = \
	docs/gcli-api.1 \
	docs/gcli-comment.1 \
	docs/gcli-config.1 \
	docs/gcli-forks.1 \
	docs/gcli-gists.1 \
	docs/gcli-issues.1 \
	docs/gcli-labels.1 \
	docs/gcli-milestones.1 \
	docs/gcli-pipelines.1 \
	docs/gcli-pulls.1 \
	docs/gcli-releases.1 \
	docs/gcli-repos.1 \
	docs/gcli-snippets.1 \
	docs/gcli-status.1 \
	docs/gcli.1 \
	docs/gcli.5

gcli_SOURCES = \
	include/gcli/cmd/attachments.h src/cmd/attachments.c \
	include/gcli/cmd/ci.h src/cmd/ci.c \
	include/gcli/cmd/cmdconfig.h src/cmd/cmdconfig.c \
	include/gcli/cmd/cmd.h src/cmd/cmd.c \
	include/gcli/cmd/colour.h src/cmd/colour.c \
	include/gcli/cmd/comment.h src/cmd/comment.c \
	include/gcli/cmd/config.h src/cmd/config.c \
	include/gcli/cmd/editor.h src/cmd/editor.c \
	include/gcli/cmd/forks.h src/cmd/forks.c \
	include/gcli/cmd/gists.h src/cmd/gists.c \
	include/gcli/cmd/gitconfig.h src/cmd/gitconfig.c \
	include/gcli/cmd/issues.h src/cmd/issues.c \
	include/gcli/cmd/labels.h src/cmd/labels.c \
	include/gcli/cmd/milestones.h src/cmd/milestones.c \
	include/gcli/cmd/pipelines.h src/cmd/pipelines.c \
	include/gcli/cmd/pulls.h src/cmd/pulls.c \
	include/gcli/cmd/releases.h src/cmd/releases.c \
	include/gcli/cmd/repos.h src/cmd/repos.c \
	include/gcli/cmd/snippets.h src/cmd/snippets.c \
	include/gcli/cmd/status.h src/cmd/status.c \
	include/gcli/cmd/status_interactive.h src/cmd/status_interactive.c \
	include/gcli/cmd/table.h src/cmd/table.c \
	include/gcli/cmd/interactive.h src/cmd/interactive.c \
	src/cmd/api.c \
	src/cmd/gcli.c

libpdjson_la_SOURCES = \
	thirdparty/pdjson/pdjson.c \
	thirdparty/pdjson/pdjson.h

TEMPLATES = \
	templates/github/api.t \
	templates/github/checks.t \
	templates/github/comments.t \
	templates/github/forks.t \
	templates/github/gists.t \
	templates/github/issues.t \
	templates/github/labels.t \
	templates/github/pulls.t \
	templates/github/releases.t \
	templates/github/repos.t \
	templates/github/status.t \
	templates/github/milestones.t \
	templates/gitlab/api.t \
	templates/gitlab/comments.t \
	templates/gitlab/forks.t \
	templates/gitlab/issues.t \
	templates/gitlab/labels.t \
	templates/gitlab/merge_requests.t \
	templates/gitlab/milestones.t \
	templates/gitlab/pipelines.t \
	templates/gitlab/releases.t \
	templates/gitlab/repos.t \
	templates/gitlab/sshkeys.t \
	templates/gitlab/status.t \
	templates/gitlab/snippets.t \
	templates/gitea/milestones.t \
	templates/gitea/status.t \
	templates/bugzilla/api.t \
	templates/bugzilla/bugs.t

headerdir = $(prefix)
nobase_header_HEADERS = include/gcli/gcli.h include/gcli/comments.h	\
	include/gcli/curl.h include/gcli/forks.h			\
	include/gcli/issues.h include/gcli/labels.h			\
	include/gcli/milestones.h include/gcli/pulls.h			\
	include/gcli/repos.h include/gcli/gitlab/snippets.h		\
	include/gcli/status.h include/gcli/sshkeys.h

pkgconfdir = $(libdir)/pkgconfig
pkgconf_DATA = libgcli.pc

libgcli_la_SOURCES = \
	include/gcli/ctx.h src/ctx.c \
	include/gcli/gcli.h src/gcli.c \
	include/gcli/date_time.h src/date_time.c \
	src/attachments.c include/gcli/attachments.h \
	src/base64.c include/gcli/base64.h \
	src/comments.c include/gcli/comments.h \
	src/curl.c include/gcli/curl.h \
	src/forges.c include/gcli/forges.h \
	src/forks.c include/gcli/forks.h \
	src/issues.c include/gcli/issues.h \
	src/json_gen.c include/gcli/json_gen.h \
	src/json_util.c include/gcli/json_util.h \
	src/labels.c include/gcli/labels.h \
	src/milestones.c include/gcli/milestones.h \
	src/nvlist.c include/gcli/nvlist.h \
	src/pulls.c include/gcli/pulls.h \
	src/releases.c include/gcli/releases.h \
	src/repos.c include/gcli/repos.h \
	src/gitlab/snippets.c include/gcli/gitlab/snippets.h \
	src/status.c include/gcli/status.h \
	src/sshkeys.c include/gcli/sshkeys.h \
	src/gitlab/api.c include/gcli/gitlab/api.h \
	src/gitlab/comments.c include/gcli/gitlab/comments.h \
	src/gitlab/config.c include/gcli/gitlab/config.h \
	src/gitlab/forks.c include/gcli/gitlab/forks.h \
	src/gitlab/issues.c include/gcli/gitlab/issues.h \
	src/gitlab/labels.c include/gcli/gitlab/labels.h \
	src/gitlab/merge_requests.c include/gcli/gitlab/merge_requests.h \
	src/gitlab/milestones.c include/gcli/gitlab/milestones.h \
	src/gitlab/pipelines.c include/gcli/gitlab/pipelines.h \
	src/gitlab/releases.c include/gcli/gitlab/releases.h \
	src/gitlab/repos.c include/gcli/gitlab/repos.h \
	src/gitlab/status.c include/gcli/gitlab/status.h \
	src/gitlab/sshkeys.c include/gcli/gitlab/sshkeys.h \
	src/github/releases.c include/gcli/github/releases.h \
	src/github/config.c include/gcli/github/config.h \
	src/github/api.c include/gcli/github/api.h \
	src/github/repos.c include/gcli/github/repos.h \
	src/github/forks.c include/gcli/github/forks.h \
	src/github/pulls.c include/gcli/github/pulls.h \
	src/github/comments.c include/gcli/github/comments.h \
	src/github/status.c include/gcli/github/status.h \
	src/github/labels.c include/gcli/github/labels.h \
	src/github/milestones.c include/gcli/github/milestones.h \
	src/github/issues.c include/gcli/github/issues.h \
	src/github/checks.c include/gcli/github/checks.h \
	src/github/gists.c include/gcli/github/gists.h \
	src/github/sshkeys.c include/gcli/github/sshkeys.h \
	src/gitea/issues.c include/gcli/gitea/issues.h \
	src/gitea/labels.c include/gcli/gitea/labels.h \
	src/gitea/forks.c include/gcli/gitea/forks.h \
	src/gitea/comments.c include/gcli/gitea/comments.h \
	src/gitea/config.c include/gcli/gitea/config.h \
	src/gitea/pulls.c include/gcli/gitea/pulls.h \
	src/gitea/releases.c include/gcli/gitea/releases.h \
	src/gitea/repos.c include/gcli/gitea/repos.h \
	src/gitea/sshkeys.c include/gcli/gitea/sshkeys.h \
	src/gitea/status.c include/gcli/gitea/status.h \
	src/gitea/milestones.c include/gcli/gitea/milestones.h \
	src/bugzilla/api.c include/gcli/bugzilla/api.h \
	src/bugzilla/attachments.c include/gcli/bugzilla/attachments.h \
	src/bugzilla/bugs.c include/gcli/bugzilla/bugs.h \
	src/bugzilla/bugs-parser.c include/gcli/bugzilla/bugs-parser.h \
	src/bugzilla/config.c include/gcli/bugzilla/config.h \
	$(TEMPLATES) \
	thirdparty/sn/sn.c thirdparty/sn/sn.h

libgcli_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DIN_LIBGCLI=1

BUILT_SOURCES = \
	$(TEMPLATES:.t=.c) \
	$(TEMPLATES:.t=.h)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	LICENSE \
	README.md  \
	HACKING.md \
	autogen.sh \
	m4/.gitkeep \
	docs/pgen.org \
	Changelog.md

###########################################
# Tests
if HAVE_TESTS
check_PROGRAMS = \
	tests/json-escape$(EXEEXT) \
	tests/github-parse-tests$(EXEEXT) \
	tests/gitlab-parse-tests$(EXEEXT) \
	tests/gitea-parse-tests$(EXEEXT) \
	tests/bugzilla-parse-tests$(EXEEXT) \
	tests/base64-tests$(EXEEXT) \
	tests/url-encode$(EXEEXT) \
	tests/pretty_print_test$(EXEEXT) \
	tests/test-jsongen$(EXEEXT)

$(check_PROGRAMS): tests/gcli_tests.h

EXTRA_DIST += tests/gcli_tests.h
check: do_test

do_test: $(check_PROGRAMS) tests/Kyuafile
	kyua test -k $(top_srcdir)/tests/Kyuafile --build-root=$(top_builddir)/tests

tests_json_escape_SOURCES = \
	tests/json-escape.c
tests_json_escape_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_json_escape_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_github_parse_tests_SOURCES = \
	tests/github-parse-tests.c
tests_github_parse_tests_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_github_parse_tests_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_gitlab_parse_tests_SOURCES = \
	tests/gitlab-parse-tests.c
tests_gitlab_parse_tests_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_gitlab_parse_tests_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_gitea_parse_tests_SOURCES = \
	tests/gitea-parse-tests.c
tests_gitea_parse_tests_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_gitea_parse_tests_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_bugzilla_parse_tests_SOURCES = \
	tests/bugzilla-parse-tests.c
tests_bugzilla_parse_tests_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_bugzilla_parse_tests_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_base64_tests_SOURCES = \
	tests/base64-tests.c
tests_base64_tests_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_base64_tests_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_url_encode_SOURCES = \
	tests/url-encode.c
tests_url_encode_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_url_encode_LDADD = \
	libgcli.la libpdjson.la \
	$(LIBATFC_LIBS)

tests_pretty_print_test_SOURCES = \
	tests/pretty_print_test.c \
	thirdparty/sn/sn.c thirdparty/sn/sn.h
tests_pretty_print_test_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_pretty_print_test_LDADD = \
	$(LIBATFC_LIBS)

tests_test_jsongen_SOURCES = \
	tests/test-jsongen.c
tests_test_jsongen_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBATFC_CFLAGS)
tests_test_jsongen_LDADD = \
	libgcli.la \
	$(LIBATFC_LIBS)

EXTRA_DIST += \
	tests/Kyuafile \
	tests/samples/github_simple_comment.json	\
	tests/samples/github_simple_fork.json		\
	tests/samples/github_simple_issue.json		\
	tests/samples/github_simple_issue.json		\
	tests/samples/github_simple_label.json		\
	tests/samples/github_simple_milestone.json	\
	tests/samples/github_simple_pull.json		\
	tests/samples/github_simple_release.json	\
	tests/samples/github_simple_repo.json		\
	tests/samples/gitlab_simple_issue.json		\
	tests/samples/gitlab_simple_fork.json		\
	tests/samples/gitlab_simple_label.json		\
	tests/samples/gitlab_simple_merge_request.json	\
	tests/samples/gitlab_simple_milestone.json	\
	tests/samples/gitlab_simple_pipeline.json	\
	tests/samples/gitlab_simple_release.json	\
	tests/samples/gitlab_simple_repo.json		\
	tests/samples/gitlab_simple_snippet.json	\
	tests/samples/gitlab_token_expired.json         \
	tests/samples/gitlab_error_unauthorised.json    \
	tests/samples/github_simple_check.json		\
	tests/samples/gitea_simple_notification.json    \
	tests/samples/bugzilla_attachments.json         \
	tests/samples/bugzilla_comments.json            \
	tests/samples/bugzilla_simple_bug.json

endif
