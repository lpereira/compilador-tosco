/*
 * Simple Pascal Compiler
 * Symbol Table
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

#include <glib.h>

typedef enum {
	ST_NONE,
	ST_PROGRAM,
	ST_VARIABLE,
	ST_FUNCTION,
	ST_PROCEDURE,
} STEntryType;

typedef enum {
	SK_NONE,
	SK_INTEGER,
	SK_BOOLEAN
} STEntryKind;

typedef struct	_STEntry		STEntry;
typedef struct	_SymbolTable	SymbolTable;

struct _STEntry {
	STEntryType	type;
	STEntryKind	kind;

	guint		label_number;
	guint		size, offset;
};

struct _SymbolTable {
	GSList *table;
};

extern gchar *st_types[], *sk_types[];

SymbolTable	*symbol_table_new(void);
void		 symbol_table_free(SymbolTable *st);

void		 symbol_table_push_context(SymbolTable *st);
void		 symbol_table_pop_context(SymbolTable *st);

void		 symbol_table_install(SymbolTable *st, gchar *symbol_name, STEntryType type, STEntryKind kind);

gboolean	 symbol_table_is_installed(SymbolTable *st, gchar *symbol_name);
STEntryType	 symbol_table_get_entry_type(SymbolTable *st, gchar *symbol_name);
STEntryKind  symbol_table_get_entry_kind(SymbolTable *st, gchar *symbol_name);

STEntryType	 symbol_table_get_entry_type_n(SymbolTable *st, gchar *symbol_name, gint n);
STEntryKind  symbol_table_get_entry_kind_n(SymbolTable *st, gchar *symbol_name, gint n);

void		 symbol_table_set_label_number(SymbolTable *st, gchar *symbol_name, guint label_number);
guint		 symbol_table_get_label_number(SymbolTable *st, gchar *symbol_name);

void		 symbol_table_set_size_and_offset(SymbolTable *st, gchar *symbol_name, guint size, guint offset);
gboolean	 symbol_table_get_size_and_offset(SymbolTable *st, gchar *symbol_name, guint *size, guint *offset);

guint		 symbol_table_get_current_offset(SymbolTable *st, gchar *symbol_name);

#endif	/* __SYMBOL_TABLE_H__ */
