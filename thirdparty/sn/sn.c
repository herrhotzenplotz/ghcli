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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sn/sn.h>

static int verbosity = VERBOSITY_NORMAL;

void
sn_setverbosity(int level)
{
	verbosity = level;
}

int
sn_getverbosity(void)
{
	return verbosity;
}


void
errx(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	exit(code);
}

void
err(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(errno));
	exit(code);
}

void
warnx(const char *fmt, ...)
{
	if (!sn_verbose())
	    return;

	fputs("warning: ", stderr);
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
}

void
warn(const char *fmt, ...)
{
	if (!sn_verbose())
		return;

	fputs("warning: ", stderr);
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(errno));
}

char *
sn_strndup(const char *it, size_t len)
{
	char *result = NULL;
	char const *tmp = NULL;
	size_t actual = 0;

	if (!len)
		return NULL;

	tmp = it;

	while (tmp[actual++] && actual < len);

	result = calloc(1, actual + 1);
	memcpy(result, it, actual);
	return result;
}

char *
sn_asprintf(const char *fmt, ...)
{
	char tmp = 0, *result = NULL;
	size_t actual = 0;
	va_list vp;

	va_start(vp, fmt);
	actual = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);

	result = calloc(1, actual + 1);
	if (!result)
		err(1, "calloc");

	va_start(vp, fmt);
	vsnprintf(result, actual + 1, fmt, vp);
	va_end(vp);

	return result;
}

int
sn_mmap_file(const char *path, void **buffer)
{
	struct stat stat_buf = {0};
	int fd = 0;

	/* Precautiously nullify the buffer, because it is better to have
	 * it point to null if the pointer variable passed here is
	 * allocated on the stack and not initialized. This will make
	 * debugging easier. */
	*buffer = NULL;

	if (access(path, R_OK) < 0)
		err(1, "access");

	if (stat(path, &stat_buf) < 0)
		err(1, "stat");

	/* we should not pass a size of 0 to mmap, as this will trigger an
	 * EINVAL. Thus we can also avoid calling open on the file and
	 * save a few resources. I discovered this error the hard way in a
	 * Haiku VM. */
	if (stat_buf.st_size == 0)
		return 0;

	if ((fd = open(path, O_RDONLY)) < 0)
		err(1, "open");

	*buffer = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (*buffer == MAP_FAILED)
		err(1, "mmap");

	return stat_buf.st_size;
}

int
sn_read_file(char const *path, char **buffer)
{
	FILE *f;
	size_t len;
	int rc = 0;

	/* open file and determine length */
	f = fopen(path, "r");
	if (!f)
		return -1;

	if (fseek(f, 0, SEEK_END) < 0)
		goto err_seek;

	len = ftell(f);
	rewind(f);

	*buffer = malloc(len + 1);
	if (fread(*buffer, 1, len, f) != len) {
		rc = -1;
		goto err_read;
	}

	(*buffer)[len] = '\0';

	rc = (int)(len);

err_read:
err_seek:
	fclose(f);

	return rc;
}

sn_sv
sn_sv_trim_front(sn_sv it)
{
	if (it.length == 0)
		return it;

	// TODO: not utf-8 aware
	while (it.length > 0) {
		if (!isspace(*it.data))
			break;

		it.data++;
		it.length--;
	}

	return it;
}

static sn_sv
sn_sv_trim_end(sn_sv it)
{
	while (it.length > 0 && isspace(it.data[it.length - 1]))
		it.length--;

	return it;
}

sn_sv
sn_sv_trim(sn_sv it)
{
	return sn_sv_trim_front(sn_sv_trim_end(it));
}

sn_sv
sn_sv_chop_until(sn_sv *it, char c)
{
	sn_sv result = *it;

	result.length = 0;

	while (it->length > 0) {

		if (*it->data == c)
			break;

		it->data++;
		it->length--;
		result.length++;
	}

	return result;
}

bool
sn_sv_has_prefix(sn_sv it, const char *prefix)
{
	size_t len = strlen(prefix);

	if (it.length < len)
		return false;

	return strncmp(it.data, prefix, len) == 0;
}

bool
sn_sv_eq(sn_sv this, sn_sv that)
{
	if (this.length != that.length)
		return false;

	return strncmp(this.data, that.data, this.length) == 0;
}

bool
sn_sv_eq_to(const sn_sv this, const char *that)
{
	size_t len = strlen(that);
	if (len != this.length)
		return false;

	return strncmp(this.data, that, len) == 0;
}

char *
sn_strip_suffix(char *it, const char *suffix)
{
	int it_len = strlen(it);
	int su_len = strlen(suffix);

	if (su_len > it_len)
		return it;

	int off = it_len - su_len;

	if (strncmp(it + off, suffix, su_len) == 0)
		it[off] = '\0';

	return it;
}

sn_sv
sn_sv_fmt(const char *fmt, ...)
{
	char tmp = 0;
	va_list vp;
	sn_sv result = {0};

	va_start(vp, fmt);

	result.length = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);
	result.data = calloc(1, result.length + 1);

	va_start(vp, fmt);
	vsnprintf(result.data, result.length + 1, fmt, vp);
	va_end(vp);

	return result;
}

char *
sn_sv_to_cstr(sn_sv it)
{
	return sn_strndup(it.data, it.length);
}

bool
sn_yesno(const char *fmt, ...)
{
	char tmp = 0;
	va_list vp;
	sn_sv message = {0};
	bool result = false;

	va_start(vp, fmt);

	message.length = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);
	message.data = calloc(1, message.length + 1);

	va_start(vp, fmt);
	vsnprintf(message.data, message.length + 1, fmt, vp);
	va_end(vp);

	do {
		printf(SV_FMT" [yN] ", SV_ARGS(message));

		char c = getchar();

		if (c == 'y' || c == 'Y') {
			result = true;
			break;
		} else if (c == '\n' || c == 'n' || c == 'N') {
			break;
		}

		getchar(); // consume newline character

	} while (!feof(stdin));

	free(message.data);
	return result;
}

sn_sv
sn_sv_strip_suffix(sn_sv input, const char *suffix)
{
	sn_sv expected_suffix = SV((char *)suffix);

	if (input.length < expected_suffix.length)
		return input;

	sn_sv actual_suffix = sn_sv_from_parts(
		input.data + input.length - expected_suffix.length,
		expected_suffix.length);

	if (sn_sv_eq(expected_suffix, actual_suffix))
		input.length -= expected_suffix.length;

	return input;
}

char *
sn_join_with(char const *const items[], size_t const items_size, char const *sep)
{
	char *buffer = NULL;
	size_t buffer_size = 0;
	size_t bufoff = 0;
	size_t sep_size = 0;

	sep_size = strlen(sep);

	/* this works because of the null terminator at the end */
	for (size_t i = 0; i < items_size; ++i) {
		buffer_size += strlen(items[i]) + sep_size;
	}

	buffer = calloc(1, buffer_size);
	if (!buffer)
		return NULL;

	for (size_t i = 0; i < items_size; ++i) {
		size_t len = strlen(items[i]);

		memcpy(buffer + bufoff, items[i], len);
		if (i != items_size - 1)
			memcpy(&buffer[bufoff + len], sep, sep_size);

		bufoff += len + sep_size;
	}

	return buffer;
}
