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

/*
 * LibSN - things I reuse all the time.
 */

#ifndef SN_H
#define SN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

enum {
	VERBOSITY_NORMAL  = 0,
	VERBOSITY_QUIET   = 1,
	VERBOSITY_VERBOSE = 2,
};

/* mostly concerning warn(x) */
void sn_setverbosity(int verbosity);
int  sn_getverbosity(void);

static inline int
sn_verbose(void)
{
	return sn_getverbosity() == VERBOSITY_VERBOSE;
}

static inline int
sn_normal(void)
{
	return sn_getverbosity() == VERBOSITY_NORMAL;
}

static inline int
sn_quiet(void)
{
	return sn_getverbosity() == VERBOSITY_QUIET;
}

/* error functions */
/* print a formatted error message and exit with code */
void errx(int code, const char *fmt, ...) PRINTF_FORMAT(2, 3);
/* print a formatted error message, the error retrieved from errno and exit with code */
void err(int code, const char *fmt, ...) PRINTF_FORMAT(2, 3);
void warnx(const char *fmt, ...) PRINTF_FORMAT(1, 2);
void warn(const char *fmt, ...) PRINTF_FORMAT(1, 2);

/* for convenience */
#define sn_unimplemented errx(42, "%s: unimplemented", __func__)
#define sn_notreached    errx(42, "%s: unreachable", __func__)

/* Weird minimum function */
static inline int
sn_min(int x, int y)
{
	if (x < 0)
		return y;
	else if (y < 0)
		return x;
	else if (x < y)
		return x;
	else
		return y;
}

/* string functions */
char *sn_strndup (const char *it, size_t len);
char *sn_asprintf(const char *fmt, ...) PRINTF_FORMAT(1, 2);
// modifies the underlying string
char *sn_strip_suffix(char *it, const char *suffix);

/* io file mapping */
int sn_mmap_file(const char *path, void **buffer);
int sn_read_file(char const *path, char **buffer);

/* stringview */
typedef struct sn_sv sn_sv;

struct sn_sv {
	char *data;
	size_t length;
};

#define SV(x) (sn_sv) { .data = x, .length = strlen(x) }
#define SV_FMT "%.*s"
#define SV_ARGS(x) (int)x.length, x.data
#define SV_NULL (sn_sv) {0}

static inline sn_sv
sn_sv_from_parts(char *buf, size_t len)
{
	return (sn_sv) { .data = buf, .length = len };
}

sn_sv sn_sv_trim_front(sn_sv);
sn_sv sn_sv_trim(sn_sv);
sn_sv sn_sv_chop_until(sn_sv *, char);
sn_sv sn_sv_chop_to_last(sn_sv *, char);
bool sn_sv_has_prefix(sn_sv, const char *);
bool sn_sv_eq(const sn_sv, const sn_sv);
bool sn_sv_eq_to(const sn_sv, const char *);
sn_sv sn_sv_fmt(const char *fmt, ...) PRINTF_FORMAT(1, 2);
char *sn_sv_to_cstr(sn_sv);
sn_sv sn_sv_strip_suffix(sn_sv, const char *suffix);

static inline bool
sn_sv_null(sn_sv it)
{
	return it.data == NULL && it.length == 0;
}

static inline bool
sn_strempty(char const *const str)
{
	if (str == NULL)
		return true;

	return *str == '\0';
}

/* interactive user functions */
bool sn_yesno(const char *fmt, ...) PRINTF_FORMAT(1, 2);

static inline const char *
sn_bool_yesno(bool x)
{
	return x ? "yes" : "no";
}

char *sn_join_with(char const *const items[], size_t const items_size, char const *sep);

#ifndef ARRAY_SIZE
#    define ARRAY_SIZE(xs) (sizeof(xs) / sizeof(xs[0]))
#endif /* ARRAY_SIZE */

#endif /* SN_H */
