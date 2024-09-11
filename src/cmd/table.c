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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>

#include <gcli/date_time.h>
#include <gcli/gcli.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sn/sn.h>

/* A row */
struct gcli_tblrow;

/* Internal state of a table printer. We return a handle to it in
 * gcli_table_init. */
struct gcli_tbl {
	struct gcli_tblcoldef const *cols; /* user provided column definitons */
	int *col_widths;                   /* minimum width of the columns */
	size_t cols_size;                  /* size of above arrays */

	struct gcli_tblrow *rows;   /* list of rows */
	size_t rows_size;           /* number of rows */
};

struct gcli_tblrow {
	struct {
		char *text;             /* the text in the cell */
		char const *colour;     /* colour (ansi escape sequence) if
		                         * explicit fixed colour was given */
	} *cells;
};

/* Push a row into the table state */
static int
table_pushrow(struct gcli_tbl *const table, struct gcli_tblrow row)
{
	table->rows = realloc(table->rows, sizeof(*table->rows) * (table->rows_size + 1));
	if (!table->rows)
		return -1;

	table->rows[table->rows_size++] = row;

	return 0;
}

/** Initialize the internal state structure of the table printer. */
gcli_tbl
gcli_tbl_begin(struct gcli_tblcoldef const *const cols, size_t const cols_size)
{
	struct gcli_tbl *tbl;

    /* Allocate the structure and fill in the handle */
	tbl = calloc(1, sizeof(*tbl));
	if (!tbl)
		return NULL;

	/* Reserve memory for the column sizes */
	tbl->col_widths = calloc(cols_size, sizeof(*tbl->col_widths));
	if (!tbl->col_widths) {
		free(tbl);
		return NULL;
	}

	/* Store the list of columns */
	tbl->cols = cols;
	tbl->cols_size = cols_size;

	/* Check the headers */
	for (size_t i = 0; i < cols_size; ++i) {

		/* Compute the header's length and use these as initial
		 * values */
		tbl->col_widths[i] = strlen(cols[i].name);
	}

	return tbl;
}

static void
table_freerow(struct gcli_tblrow *row, size_t const cols)
{
	for (size_t i = 0; i < cols; ++i)
		free(row->cells[i].text);

	free(row->cells);
	row->cells = NULL;
}

static int
tablerow_add_cell(struct gcli_tbl *const table,
                  struct gcli_tblrow *const row,
                  size_t const col, va_list *vp)
{
	int cell_size = 0;

	/* Extract the explicit colour code */
	if (table->cols[col].flags & GCLI_TBLCOL_COLOUREXPL) {
		int code = va_arg(*vp, int);

		/* don't free that! it's allocated and free'ed inside colour.c */
		row->cells[col].colour = gcli_setcolour(code);
	} else if (table->cols[col].flags & GCLI_TBLCOL_256COLOUR) {
		uint64_t hexcode = va_arg(*vp, uint64_t);

		/* see comment above */
		row->cells[col].colour = gcli_setcolour256(hexcode);
	}


	/* Process the content */
	switch (table->cols[col].type) {
	case GCLI_TBLCOLTYPE_INT: {
		row->cells[col].text = sn_asprintf("%d", va_arg(*vp, int));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_ID: {
		row->cells[col].text = sn_asprintf("%"PRIid, va_arg(*vp, uint64_t));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_LONG: {
		row->cells[col].text = sn_asprintf("%ld", va_arg(*vp, long));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_STRING: {
		char *it = va_arg(*vp, char *);
		if (!it)
			it = "<empty>"; /* hack */
		row->cells[col].text = strdup(it);
		cell_size = strlen(it);
	} break;
	case GCLI_TBLCOLTYPE_DOUBLE: {
		row->cells[col].text = sn_asprintf("%lf", va_arg(*vp, double));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_BOOL: {
		/* Do not use real _Bool type as it triggers a compiler bug in
		 * LLVM clang 13 */
		int val = va_arg(*vp, int);
		if (val) {
			row->cells[col].text = strdup("yes");
			cell_size = 3;
		} else {
			row->cells[col].text = strdup("no");
			cell_size = 2;
		}
	} break;
	default:
		return -1;
	}

	/* Update the column width if needed */
	if (table->col_widths[col] < cell_size)
		table->col_widths[col] = cell_size;

	return 0;
}

int
gcli_tbl_add_row(gcli_tbl _table, ...)
{
	va_list vp;
	struct gcli_tblrow row = {0};
	struct gcli_tbl *table = (struct gcli_tbl *)(_table);

	/* reserve array of cells */
	row.cells = calloc(table->cols_size, sizeof(*row.cells));
	if (!row.cells)
		return -1;

	va_start(vp, _table);

	/* Step through all the columns and print the cells */
	for (size_t i = 0; i < table->cols_size; ++i) {
		if (tablerow_add_cell(table, &row, i, &vp) < 0) {
			table_freerow(&row, table->cols_size);
			va_end(vp);
			return -1;
		}
	}

	va_end(vp);

	/* Push the row into the table */
	if (table_pushrow(table, row) < 0) {
		table_freerow(&row, table->cols_size);
		return -1;
	}

	return 0;
}

static void
pad(size_t const n)
{
	for (size_t p = 0; p < n; ++p)
		putchar(' ');
}

static void
dump_row(struct gcli_tbl const *const table, size_t const i)
{
	struct gcli_tblrow const *const row = &table->rows[i];

	for (size_t col = 0; col < table->cols_size; ++col) {

		/* Skip empty columns (as with colour indicators in no-colour
		 * mode) */
		if (table->col_widths[col] == 0)
			continue;

		/* If right justified and not last column, print padding */
		if ((table->cols[col].flags & GCLI_TBLCOL_JUSTIFYR) &&
		    (col + 1) < table->cols_size)
			pad(table->col_widths[col] - strlen(row->cells[col].text));

		/* State colour */
		if (table->cols[col].flags & GCLI_TBLCOL_STATECOLOURED)
			printf("%s", gcli_state_colour_str(row->cells[col].text));
		else if (table->cols[col].flags &
		         (GCLI_TBLCOL_COLOUREXPL|GCLI_TBLCOL_256COLOUR))
			printf("%s", row->cells[col].colour);

		/* Bold */
		if (table->cols[col].flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_setbold());

		/* Print cell if it is not NULL, otherwise indicate it by
		 * printing <empty> */
		printf("%s", row->cells[col].text ? row->cells[col].text : "<empty>");

		/* End colour */
		if (table->cols[col].flags &
		    (GCLI_TBLCOL_STATECOLOURED
		     |GCLI_TBLCOL_COLOUREXPL
		     |GCLI_TBLCOL_256COLOUR))
			printf("%s", gcli_resetcolour());

		/* Stop printing in bold */
		if (table->cols[col].flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_resetbold());

		/* If not last column, print padding of 2 spaces */
		if ((col + 1) < table->cols_size) {
			size_t padding =
				(table->cols[col].flags & GCLI_TBLCOL_TIGHT)
				? 1 : 2;

			/* If left-justified, print justify-padding */
			if (!(table->cols[col].flags & GCLI_TBLCOL_JUSTIFYR) &&
			    (col + 1) < table->cols_size)
				padding += table->col_widths[col] - strlen(row->cells[col].text);

			pad(padding);
		}
	}
	putchar('\n');
}

static int
gcli_tbl_dump(gcli_tbl const _table)
{
	struct gcli_tbl const *const table = (struct gcli_tbl const *const)_table;

	for (size_t i = 0; i < table->cols_size; ++i) {
		size_t padding = 0;
		/* Skip empty columns e.g. in no-colour mode */
		if (table->col_widths[i] == 0)
			continue;

		/* Check if we have tight column spacing */
		if (table->cols[i].flags & GCLI_TBLCOL_TIGHT)
			padding = 1;
		else
			padding = 2;

		printf("%s", table->cols[i].name);

		if ((i + 1) < table->cols_size)
			pad(padding + table->col_widths[i] - strlen(table->cols[i].name));
	}
	printf("\n");

	for (size_t i = 0; i < table->rows_size; ++i) {
		dump_row(table, i);
	}

	return 0;
}

static void
gcli_tbl_free(gcli_tbl _table)
{
	struct gcli_tbl *tbl = (struct gcli_tbl *)_table;

	for (size_t row = 0; row < tbl->rows_size; ++row)
		table_freerow(&tbl->rows[row], tbl->cols_size);

	free(tbl->rows);
	free(tbl->col_widths);
	free(tbl);
}

void
gcli_tbl_end(gcli_tbl tbl)
{
	gcli_tbl_dump(tbl);
	gcli_tbl_free(tbl);
}

/* DICTIONARY *********************************************************/
struct gcli_dict {
	struct gcli_dict_entry {
		char *key;
		char *value;
		int flags;
		uint32_t colour_args;
	} *entries;
	size_t entries_size;

	size_t max_key_len;

	struct gcli_ctx *ctx;
};

/* Create a new long list printer and return a handle to it */
gcli_dict
gcli_dict_begin(void)
{
	return calloc(1, sizeof(struct gcli_dict));
}

static int
gcli_dict_add_row(struct gcli_dict *list,
                  char const *const key,
                  int flags,
                  int colour_args,
                  char *value)
{
	struct gcli_dict_entry *entry;
	size_t keylen;

	list->entries = realloc(list->entries,
	                        sizeof(*list->entries) * (list->entries_size + 1));
	if (!list->entries)
		return -1;

	entry = &list->entries[list->entries_size++];

	entry->key = strdup(key);
	entry->value = value;
	entry->flags = flags;
	entry->colour_args = colour_args;

	if ((keylen = strlen(key)) > list->max_key_len)
		list->max_key_len = keylen;

	return 0;
}

int
gcli_dict_add(gcli_dict list,
              char const *const key,
              int flags,
              uint32_t colour_args,
              char const *const fmt,
              ...)
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

    return gcli_dict_add_row(list, key, flags, colour_args, result);
}

int
gcli_dict_add_string(gcli_dict list,
                     char const *const key,
                     int flags,
                     uint32_t colour_args,
                     char const *const str)
{
	return gcli_dict_add_row(list, key, flags, colour_args,
	                         strdup(str ? str : "<empty>"));
}

int
gcli_dict_add_timestamp(gcli_dict list, char const *key, int flags,
                        uint32_t colour_args, time_t stamp)
{
	char *tmp = NULL;
	int rc = 0;

	rc = gcli_format_as_localtime(g_clictx, stamp, &tmp);
	if (rc < 0)
		return rc;

	return gcli_dict_add_row(list, key, flags, colour_args, tmp);
}

int
gcli_dict_add_sv_list(gcli_dict dict,
                      char const *const key,
                      sn_sv const *const list,
                      size_t const list_size)
{
	size_t totalsize = 0;
	char *catted, *hd;

	/* Sum of string lengths */
	for (size_t i = 0; i < list_size; ++i)
		totalsize += list[i].length;

	/* Account for comma and space between each */
	totalsize += (list_size - 1) * 2;

	/* concatenate the strings */
	hd = catted = calloc(totalsize + 1, 1);
	for (size_t i = 0; i < list_size; ++i) {
		memcpy(hd, list[i].data, list[i].length);
		hd += list[i].length;

		if (i + 1 < list_size) {
			strcat(catted, ", ");
			hd += 2;
		}
	}

	/* Push the row into the state */
	return gcli_dict_add_row(dict, key, 0, 0, catted);
}

int
gcli_dict_add_string_list(gcli_dict dict, char const *const key,
                          char const *const *list, size_t const list_size)
{
	char *catted = sn_join_with(
		(char const *const *)list, list_size, ", "); /* yolo */

	/* Push the row into the state */
	return gcli_dict_add_row(dict, key, 0, 0, catted);
}

static void
gcli_dict_free(struct gcli_dict *list)
{
	for (size_t i = 0; i < list->entries_size; ++i) {
		free(list->entries[i].key);
		free(list->entries[i].value);
	}

	free(list->entries);
	free(list);
}

int
gcli_dict_end(gcli_dict _list)
{
	struct gcli_dict *list = _list;

	for (size_t i = 0; i < list->entries_size; ++i) {
		int flags = list->entries[i].flags;

		pad(list->max_key_len - strlen(list->entries[i].key));
		printf("%s : ", list->entries[i].key);

		if (flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_setbold());

		if (flags & GCLI_TBLCOL_COLOUREXPL)
			printf("%s", gcli_setcolour(list->entries[i].colour_args));

		if (flags & GCLI_TBLCOL_STATECOLOURED)
			printf("%s", gcli_state_colour_str(list->entries[i].value));

		if (flags & GCLI_TBLCOL_256COLOUR)
			printf("%s", gcli_setcolour256(list->entries[i].colour_args));

		puts(list->entries[i].value);

		if (flags & (GCLI_TBLCOL_COLOUREXPL
		             |GCLI_TBLCOL_STATECOLOURED
		             |GCLI_TBLCOL_256COLOUR))
			printf("%s", gcli_resetcolour());

		if (flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_resetbold());
	}

	gcli_dict_free(list);
	return 0;
}
