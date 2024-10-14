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

#ifndef GCLI_CMD_TABLE_H
#define GCLI_CMD_TABLE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <stdlib.h>

#include <gcli/gcli.h>

#include <sn/sn.h>

typedef void *gcli_tbl;

typedef void *gcli_dict;

/** Flags for table column definitions */
enum gcli_tblcol_flags {
	/* column is as string and colour is derived from its contents. */
	GCLI_TBLCOL_STATECOLOURED  = 1,
	/* Right-justify the column */
	GCLI_TBLCOL_JUSTIFYR       = 2,
	/* Make it bold */
	GCLI_TBLCOL_BOLD           = 4,
	/* Explicit colour - provide the colour to gcli_tbl_add_row first
	 * and second the content of the cell. */
	GCLI_TBLCOL_COLOUREXPL     = 8,
	/* 256 colour handling. Just like the above */
	GCLI_TBLCOL_256COLOUR      = 16,
	/* Have a column spacing to the right of one instead of two spaces */
	GCLI_TBLCOL_TIGHT          = 32,
};

enum gcli_tblcoltype {
	GCLI_TBLCOLTYPE_INT,        /* integer */
	GCLI_TBLCOLTYPE_LONG,       /* signed long int */
	GCLI_TBLCOLTYPE_ID,         /* some ID type (uint64_t) */
	GCLI_TBLCOLTYPE_STRING,     /* C string */
	GCLI_TBLCOLTYPE_DOUBLE,     /* double precision float */
	GCLI_TBLCOLTYPE_BOOL,       /* yes/no */
};

/** A single table column */
struct gcli_tblcoldef {
	char const *name;           /* name of the column, also displayed in first row */
	int type;                   /* type of values in this column */
	int flags;                  /* flags about this column */
};

/* Init a table printer */
gcli_tbl gcli_tbl_begin(struct gcli_tblcoldef const *cols,
                        size_t cols_size);

/* Print the table contents and free all the resources allocated in
 * the table */
void gcli_tbl_end(gcli_tbl table);

/* Add a single to an initialized table */
int gcli_tbl_add_row(gcli_tbl table, ...);

gcli_dict gcli_dict_begin(void);

int gcli_dict_add(gcli_dict list, char const *key, int flags,
                  uint32_t colour_args, char const *fmt, ...)
                  PRINTF_FORMAT(5, 6);

int gcli_dict_add_string(gcli_dict list, char const *key, int flags,
                         uint32_t colour_args, char const *str);

int gcli_dict_add_sv_list(gcli_dict dict, char const *key, sn_sv const *list,
                          size_t list_size);

int gcli_dict_add_string_list(gcli_dict dict, char const *const key,
                              char const *const *list, size_t const list_size);

int gcli_dict_end(gcli_dict _list);

#endif /* GCLI_CMD_TABLE_H */
