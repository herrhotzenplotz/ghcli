# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gcli],
        [2.4.0-devel],
        [~herrhotzenplotz/gcli-discuss@lists.sr.ht],
        [gcli],
        [https://herrhotzenplotz.de/gcli])
AM_INIT_AUTOMAKE([1.0 foreign subdir-objects dist-bzip2 dist-xz -Wall])

dnl Release Date.
PACKAGE_DATE="UNRELEASED"
AC_SUBST([PACKAGE_DATE])

dnl Silent by default.
AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_PROG_AR

LT_INIT([pic-only])
AC_SUBST([LIBTOOL_DEPS])

AC_PROG_CC([cc clang gcc cl])
AC_LANG([C])
AC_PROG_YACC
AC_PROG_LEX(noyywrap)

dnl Produce a better error message if we don't have pkg-config's macros
dnl Inspired by the way dpkg solves the problem
m4_ifndef([PKG_PROG_PKG_CONFIG], [m4_fatal([pkgconf macros missing. do you have pkgconf installed?])])
PKG_PROG_PKG_CONFIG

dnl Use ccache if found
CCACHE=""
AC_CHECK_PROG([CCACHE], [ccache], [ccache])
if ! test -z "${CCACHE}"; then
	CC="${CCACHE} ${CC}"
fi

dnl ####################################################################################
dnl LIBCURL
dnl ####################################################################################
dnl Go looking for libcurl
OPT_LIBCURL=check
AC_ARG_WITH([libcurl],
[AS_HELP_STRING([--with-libcurl[[=DIR]]],
                [Give an alternate path to libcurl.])],
OPT_LIBCURL=$withval
)

AS_IF([test "x$OPT_LIBCURL" = "xno"],
      [AC_MSG_ERROR([--with-libcurl must not be disabled])])
AS_IF([test "x$OPT_LIBCURL" = "xcheck" || test "x$OPT_LIBCURL" = "xyes"],
      [PKG_CHECK_MODULES([LIBCURL], [libcurl],,[AC_MSG_ERROR([Could not find libcurl])])
       CFLAGS="$LIBCURL_CFLAGS $CFLAGS"
       LDFLAGS="$LIBCURL_LIBS $LDFLAGS"],
      [CPPFLAGS="-I$OPT_LIBCURL/include $CPPFLAGS"
       LDFLAGS="-L$OPT_LIBCURL/lib $LDFLAGS"])
AC_CHECK_HEADER([curl/curl.h],,[AC_MSG_ERROR([Cannot find libcurl headers])])

dnl FIXME find a better way for this
dnl AC_CHECK_LIB([curl],[curl_easy_init],,[AC_MSG_ERROR([-lcurl doesn not contain curl_easy_init])])

dnl ####################################################################################
dnl LIBEDIT
dnl ####################################################################################
dnl Check for libedit
OPT_LIBEDIT=check
AC_ARG_WITH([libedit],
            [AS_HELP_STRING([--with-libedit[[=DIR]]],
                            [Use libedit at the given prefix for interactive editing])],
            OPT_LIBEDIT=$withval,
            OPT_LIBEDIT=no
)

HAVE_LIBEDIT=0
if test "x$OPT_LIBEDIT" = "xyes" || test "x$OPT_LIBEDIT" = "xcheck"; then
	HAVE_LIBEDIT=1

	PKG_CHECK_MODULES([LIBEDIT], [libedit],,[AC_MSG_ERROR([Could not find libedit])])
	CFLAGS="$LIBEDIT_CFLAGS $CFLAGS"
	LDFLAGS="$LIBEDIT_LIBS $LDFLAGS"
elif ! test "x$OPT_LIBEDIT" = "xno"; then
	HAVE_LIBEDIT=1

	CFLAGS="-I$OPT_LIBEDIT/include $CFLAGS"
	LDFLAGS="-L$OPT_LIBEDIT/lib -ledit $LDFLAGS"
fi

if test $HAVE_LIBEDIT -eq 1; then
	AC_CHECK_HEADER([histedit.h],,[AC_MSG_ERROR([Cannot find libedit headers])])
fi
AC_DEFINE_UNQUOTED([HAVE_LIBEDIT], [$HAVE_LIBEDIT], [Define if we link against libedit])

dnl ####################################################################################
dnl READLINE (second option)
dnl ####################################################################################
if ! test $HAVE_LIBEDIT -eq 1; then
	OPT_READLINE=check
	AC_ARG_WITH([readline],
	            [AS_HELP_STRING([--with-readline[[=DIR]]],
	                            [Use readline at the given prefix for interactive editing.
                                     When libedit is available this option is a no-op.])],
	            OPT_READLINE=$withval,
	            OPT_READLINE=no
	)

	HAVE_READLINE=0
	if test "x$OPT_READLINE" = "xyes" || test "x$OPT_READLINE" = "xcheck"; then
		HAVE_READLINE=1
		# FIXME: This is an ugly hack because the readline headers are
		#        not C99 clean. Clang seems to not like them and causes
		#        the configure script to fail.
		CFLAGS="${CFLAGS} -Wno-strict-prototypes"

		PKG_CHECK_MODULES([READLINE], [readline],,[AC_MSG_ERROR([Could not find readline])])
		CFLAGS="$READLINE_CFLAGS $CFLAGS"
		LDFLAGS="$READLINE_LIBS $LDFLAGS"
	elif ! test "x$OPT_READLINE" = "xno"; then
		HAVE_READLINE=1

		CFLAGS="-I$OPT_READLINE/include $CFLAGS"
		LDFLAGS="-L$OPT_READLINE/lib -lreadline $LDFLAGS"
	fi

	if test $HAVE_READLINE -eq 1; then
		AC_CHECK_HEADER([readline/readline.h],,[AC_MSG_ERROR([Cannot find readline headers])])
	fi
	AC_DEFINE_UNQUOTED([HAVE_READLINE], [$HAVE_READLINE], [Define if we link against readline])
fi

dnl ####################################################################################
dnl TEST SUITE STUFF
dnl ####################################################################################

dnl For the test suite we require libatf-c and Kyua
OPT_LIBATFC=check
AC_ARG_WITH([libatf-c],
[AS_HELP_STRING([--with-libatf-c[[=DIR]]],
                [Give an alternate path to libatf-c.])],
OPT_LIBATFC=$withval
)

HAVE_ATFC=no
if test "x$OPT_LIBATFC" = "xcheck" || test "x$OPT_LIBATFC" = "xyes"
then
        PKG_CHECK_MODULES([LIBATFC], [atf-c], [HAVE_ATFC=yes],[HAVE_ATFC=no])
elif test "x$OPT_LIBATFC" = "xno"
then
        HAVE_ATFC=no
else
        LIBATFC_CFLAGS="-I$OPT_LIBATFC/include"
        LIBATFC_LIBS="-L$OPT_LIBATFC/lib"
        HAVE_ATFC=yes
fi

if test "x$HAVE_ATFC" = "xyes"; then
	AC_SUBST([LIBATFC_CFLAGS])
	AC_SUBST([LIBATFC_LIBS])

	_OLD_CFLAGS="$CFLAGS"
	_OLD_LDFLAGS="$LDFLAGS"

	CFLAGS="$CFLAGS $LIBATFC_CFLAGS"
	LDFLAGS="$LDFLAGS $LIBATFC_LIBS"

	AC_CHECK_HEADER([atf-c.h],,[AC_MSG_ERROR([Cannot find libatf-c headers])])

	CFLAGS="$_OLD_CFLAGS"
	LDFLAGS="$_OLD_LDFLAGS"

	HAVE_ATFC=yes
fi

AC_CHECK_PROG([KYUA], [kyua], [kyua])
AC_CHECK_PROGS([REALPATH], [realpath grealpath], [])
HAVE_TESTS=0
AS_IF([test -z "$KYUA" || test "x$HAVE_ATFC" = xno || test -z "$REALPATH"],
      [HAVE_TESTS=no],
      [HAVE_TESTS=yes])
AC_SUBST([HAVE_TESTS])
AM_CONDITIONAL([HAVE_TESTS], [test "x$HAVE_TESTS" = xyes])

dnl Go looking for headers that may define getopt_long
AC_CHECK_HEADERS([getopt.h unistd.h])
dnl Other headers that we require
AC_CHECK_HEADERS([string.h signal.h sys/wait.h unistd.h sys/mman.h sys/types.h sys/queue.h],
                 ,
                 [AC_MSG_ERROR([Required header not found])])

dnl Check that the function is defined somewhere.
AC_CHECK_FUNC([getopt_long])

dnl Host OS name in various files
AC_DEFINE_UNQUOTED([HOSTOS], ["$host"], [Define to the triplet of the host operating system])

dnl Generate and substitute various files
AC_CONFIG_FILES([Makefile
                 libgcli.pc
                 docs/gcli-api.1
                 docs/gcli-comment.1
                 docs/gcli-config.1
                 docs/gcli-forks.1
                 docs/gcli-gists.1
                 docs/gcli-issues.1
                 docs/gcli-labels.1
                 docs/gcli-milestones.1
                 docs/gcli-pipelines.1
                 docs/gcli-pulls.1
                 docs/gcli-releases.1
                 docs/gcli-repos.1
                 docs/gcli-snippets.1
                 docs/gcli-status.1
                 docs/gcli.1
                 docs/gcli.5])

dnl Technically only needed if tests are enabled but this doesn't
dnl hurt.
if test "x$HAVE_TESTS" = "xyes"; then
	TESTSRCDIR="$($REALPATH ${srcdir}/tests)"
	AC_SUBST([TESTSRCDIR])
	AC_DEFINE_UNQUOTED([TESTSRCDIR], ["$TESTSRCDIR"], [Directory to the test sources])
fi

AC_OUTPUT
