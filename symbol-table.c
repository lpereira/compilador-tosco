#include "symbol-table.h"

gchar *st_types[] = { "", "variable", "function", "procedure" };
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
	GList *list;
	
	g_return_if_fail(st);
	
	for (list = st->table; list; list = list->next) {
		g_hash_table_destroy((GHashTable *) list->data);
	}
	
	g_list_free(st->table);
	g_free(st);
}

void
symbol_table_push_context(SymbolTable *st)
{
	GHashTable *ht;
	
	g_return_if_fail(st);
	
//	puts("/*push context*/");
	
	ht = g_hash_table_new(g_str_hash, g_str_equal);
	st->table = g_list_prepend(st->table, ht);
}

void
symbol_table_pop_context(SymbolTable *st)
{
	GList *first;
	
	g_return_if_fail(st);

//	puts("/*pop context*/");
	
	first = st->table;
	st->table = first->next;
	
	g_hash_table_destroy((GHashTable *) first->data);
	g_list_free_1(first);
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

//	g_print("/*install (%s,%s,%s)*/\n", symbol_name, st_types[type], sk_types[kind]);
	
	ht = (GHashTable *)st->table->data;
	
	entry = g_new0(STEntry, 1);
	entry->type = type;
	entry->kind = kind;
	
	g_hash_table_insert(ht, symbol_name, entry);
}

static STEntry *
__symbol_table_get_symbol_entry(SymbolTable *st, gchar *symbol_name, gint n)
{
	GList *list;
	STEntry *entry;
	
	g_return_val_if_fail(st, NULL);
	g_return_val_if_fail(st->table, NULL);
	
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
