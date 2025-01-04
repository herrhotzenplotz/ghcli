/*
 * Copyright 2023-2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/json_gen.h>

#include <gcli/json_util.h>

#include <config.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
grow_buffer(struct gcli_jsongen *gen)
{
	gen->buffer_capacity *= 2;
	gen->buffer = realloc(gen->buffer, gen->buffer_capacity);
}

int
gcli_jsongen_init(struct gcli_jsongen *gen)
{
	/* This will allocate a 32 byte buffer. We can optimise
	 * this for better allocation speed by analysing some statistics
	 * of how long usually the generated buffers are. */

	memset(gen, 0, sizeof(*gen));
	gen->buffer_capacity = 16;

	grow_buffer(gen);

	gen->first_elem = true;

	return 0;
}

void
gcli_jsongen_free(struct gcli_jsongen *gen)
{
	free(gen->buffer);
	gen->buffer = NULL;
	gen->buffer_size = 0;
	gen->buffer_capacity = 0;

	gen->scopes_size = 0;
}

char *
gcli_jsongen_to_string(struct gcli_jsongen *gen)
{
	char *buf = calloc(gen->buffer_size + 1, 1);

	return memcpy(buf, gen->buffer, gen->buffer_size);
}

static void
fit(struct gcli_jsongen *gen, size_t const n_chars)
{
	while (gen->buffer_capacity - gen->buffer_size < n_chars)
		grow_buffer(gen);
}

static int
push_scope(struct gcli_jsongen *gen, int const scope)
{
	if (gen->scopes_size >= (sizeof(gen->scopes) / sizeof(*gen->scopes)))
		return -1;

	gen->scopes[gen->scopes_size++] = scope;

	return 0;
}

static int
pop_scope(struct gcli_jsongen *gen)
{
	if (gen->scopes_size == 0)
		return -1;

	return gen->scopes[--gen->scopes_size];
}

static bool
is_array_or_object_scope(struct gcli_jsongen *gen)
{
	return !!gen->scopes_size;
}

static void
append_str(struct gcli_jsongen *gen, char const *str)
{
	size_t const len = strlen(str);
	fit(gen, len);
	memcpy(gen->buffer + gen->buffer_size, str, len);
	gen->buffer_size += len;
}

static void
put_comma_if_needed(struct gcli_jsongen *gen)
{
	if (!gen->await_object_value && !gen->first_elem && is_array_or_object_scope(gen))
		append_str(gen, ", ");

	gen->first_elem = false;
}

static bool
is_object_scope(struct gcli_jsongen *gen)
{
	if (gen->scopes_size == 0)
		return false;

	return gen->scopes[gen->scopes_size - 1] == GCLI_JSONGEN_OBJECT;
}

int
gcli_jsongen_begin_object(struct gcli_jsongen *gen)
{
	/* Cannot put a json object into a json object key */
	if (is_object_scope(gen) && !gen->await_object_value) {
		assert(0 && "attempt to use json object as object key");
		return -1;
	}

	put_comma_if_needed(gen);

	if (push_scope(gen, GCLI_JSONGEN_OBJECT) < 0)
		return -1;

	append_str(gen, "{");

	gen->first_elem = true;

	return 0;
}

int
gcli_jsongen_end_object(struct gcli_jsongen *gen)
{
	if (pop_scope(gen) != GCLI_JSONGEN_OBJECT) {
		assert(0 && "unbalanced json scopes");
		return -1;
	}

	append_str(gen, "}");

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}

int
gcli_jsongen_begin_array(struct gcli_jsongen *gen)
{
	/* Cannot put a json array into a json object key */
	if (is_object_scope(gen) && !gen->await_object_value) {
		assert(0 && "attempt to use array as object key");
		return -1;
	}

	put_comma_if_needed(gen);

	if (push_scope(gen, GCLI_JSONGEN_ARRAY) < 0)
		return -1;

	append_str(gen, "[");

	gen->first_elem = true;

	return 0;
}

int
gcli_jsongen_end_array(struct gcli_jsongen *gen)
{
	if (pop_scope(gen) != GCLI_JSONGEN_ARRAY) {
		assert(0 && "unbalanced json scopes");
		return -1;
	}

	append_str(gen, "]");

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}

static void
append_vstrf(struct gcli_jsongen *gen, char const *const fmt, va_list vp)
{
	va_list vp_copy;
	size_t len;

	va_copy(vp_copy, vp);
	len = vsnprintf(NULL, 0, fmt, vp_copy);

	fit(gen, len + 1);
	vsnprintf(gen->buffer + gen->buffer_size, len + 1, fmt, vp);

	gen->buffer_size += len;
}

static void
append_strf(struct gcli_jsongen *gen, char const *const fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	append_vstrf(gen, fmt, ap);
	va_end(ap);
}

int
gcli_jsongen_objmember(struct gcli_jsongen *gen, char const *const key)
{
	if (!is_object_scope(gen)) {
		assert(0 && "gcli_jsongen_objmember called outside object scope");
		return -1;
	}

	put_comma_if_needed(gen);
	char *const e_key = gcli_json_escape_cstr(key);

	append_strf(gen, "\"%s\": ", e_key);

	gen->first_elem = false;
	gen->await_object_value = true;

	free(e_key);

	return 0;
}

int
gcli_jsongen_number(struct gcli_jsongen *gen, long long const number)
{
	put_comma_if_needed(gen);
	append_strf(gen, "%lld", number);

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}

int
gcli_jsongen_id(struct gcli_jsongen *gen, gcli_id const id)
{
	put_comma_if_needed(gen);
	append_strf(gen, "%"PRIid, id);

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}

int
gcli_jsongen_bool(struct gcli_jsongen *gen, bool const value)
{
	put_comma_if_needed(gen);

	append_strf(gen, "%s", value ? "true" : "false");

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}

int
gcli_jsongen_string(struct gcli_jsongen *gen, char const *value)
{
	put_comma_if_needed(gen);
	char *const e_value = gcli_json_escape_cstr(value);

	append_strf(gen, "\"%s\"", e_value);

	gen->await_object_value = false;
	gen->first_elem = false;

	free(e_value);

	return 0;
}

int
gcli_jsongen_null(struct gcli_jsongen *gen)
{
	put_comma_if_needed(gen);
	append_str(gen, "null");

	gen->await_object_value = false;
	gen->first_elem = false;

	return 0;
}
