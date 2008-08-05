/*
 * Simple Pascal Compiler
 * Symbol Table
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#include "symbol-table.h"

gchar *st_types[] = { "", "program", "variable", "function", "procedure" };
gchar *sk_types[] = { "", "integer", "boolean" };

SymbolTable	*
symbol_table_new(void)
{
	SymbolTable *st;

	st = g_new0(SymbolTable, 1);
	st->table = NULL;
	
	symbol_table_push_context(st);
	
	return st;
}

void
symbol_table_free(SymbolTable *st)
{
	GSList *list;
	
	g_return_if_fail(st);
	
	for (list = st->table; list; list = list->next) {
		g_hash_table_unref((GHashTable *) list->data);
	}
	
	g_slist_free(st->table);
	g_free(st);
}

void
symbol_table_push_context(SymbolTable *st)
{
	GHashTable *ht;
	
	g_return_if_fail(st);
	
	ht = g_hash_table_new(g_str_hash, g_str_equal);	
	g_hash_table_ref(ht);
	
	st->table = g_slist_prepend(st->table, ht);
}

void
symbol_table_pop_context(SymbolTable *st)
{
	GSList *first;
	
	g_return_if_fail(st);

	first = st->table;
	st->table = first->next;
	
	g_hash_table_destroy((GHashTable *) first->data);
	g_slist_free_1(first);
}

void
symbol_table_install(SymbolTable *st, gchar *symbol_name, STEntryType type, STEntryKind kind)
{
	GHashTable *ht;
	STEntry *entry;
	
	g_return_if_fail(st);
	g_return_if_fail(st->table);

	if (symbol_table_is_installed(st, symbol_name))
		return;

	ht = (GHashTable *)st->table->data;
	
	entry = g_new0(STEntry, 1);
	entry->type = type;
	entry->kind = kind;
	
	g_hash_table_insert(ht, symbol_name, entry);
}

static STEntry *
__symbol_table_get_symbol_entry(SymbolTable *st, gchar *symbol_name, gint n)
{
	GSList *list;
	STEntry *entry;
	
	g_return_val_if_fail(st, NULL);

	if (!st->table)
		return NULL;
	
	if (n == -1) {
		for (list = st->table; list; list = list->next) {
			if ((entry = g_hash_table_lookup((GHashTable *)list->data, symbol_name))) {
				return entry;
			}
		}
	} else {
		for (list = st->table; n-- && list; list = list->next) {	
			if ((entry = g_hash_table_lookup((GHashTable *)list->data, symbol_name))) {
				return entry;
			}
		}
	}
	
	return NULL;
}

gboolean
symbol_table_is_installed(SymbolTable *st, gchar *symbol_name)
{
	return __symbol_table_get_symbol_entry(st, symbol_name, -1) != NULL;
}

STEntryType
symbol_table_get_entry_type(SymbolTable *st, gchar *symbol_name)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		return entry->type;
	}
	
	return ST_NONE;
}

STEntryKind
symbol_table_get_entry_kind(SymbolTable *st, gchar *symbol_name)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		return entry->kind;
	}
	
	return SK_NONE;
}

STEntryType
symbol_table_get_entry_type_n(SymbolTable *st, gchar *symbol_name, gint n)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, n))) {
		return entry->type;
	}
	
	return ST_NONE;
}

STEntryKind
symbol_table_get_entry_kind_n(SymbolTable *st, gchar *symbol_name, gint n)
{
	STEntry *entry;
	
	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, n))) {
		return entry->kind;
	}

	return SK_NONE;
}

void
symbol_table_set_label_number(SymbolTable *st, gchar *symbol_name, guint label_number)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		entry->label_number = label_number;
		return;
	}
}

guint
symbol_table_get_label_number(SymbolTable *st, gchar *symbol_name)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		return entry->label_number;
	}

	return -1;
}

static void
__update_offset_fn(gpointer key, gpointer value, guint *offset)
{
	STEntry *e = (STEntry *)value;
	*offset += e->size;
}

guint 
symbol_table_get_current_offset(SymbolTable *st, gchar *symbol_name)
{
	guint offset = 0;
	GSList *list;
	STEntry *entry;

	list = st->table;
	
	/* the variable is in the current context: the offset is zero */
	if ((entry = g_hash_table_lookup((GHashTable *)list->data, symbol_name))) {
		return 0;
	}

	/* it's not on the current context; sum up the sizes for all previously
	   declared variables to obtain our offset */
	for (list = list->next; list->next; list = list->next) {
		g_hash_table_foreach((GHashTable *)list->data,
				     (GHFunc)__update_offset_fn,
				     (gpointer) &offset);
	}
	
	return offset;
}

void
symbol_table_set_size_and_offset(SymbolTable *st, gchar *symbol_name, guint size, guint offset)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		entry->size = size;
		entry->offset = offset;
	}
}

gboolean
symbol_table_get_size_and_offset(SymbolTable *st, gchar *symbol_name, guint *size, guint *offset)
{
	STEntry *entry;

	if ((entry = __symbol_table_get_symbol_entry(st, symbol_name, -1))) {
		*size = entry->size;
		*offset = entry->offset;

		return TRUE;
	}

	return FALSE;
}
