/*
 * Simple Pascal Compiler
 * Symbol Table
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 */
#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

extern const gchar *symbol_types[];
extern const gchar *symbol_subtypes[];

typedef struct _SymbolTable	SymbolTable;
typedef struct _Symbol		Symbol;

typedef enum {
  ST_NONE,
  ST_VARIABLE,
  ST_FUNCTION,
  ST_PROCEDURE,
  ST_PROGRAM
} SymbolType;

typedef enum {
  STF_TYPE,
  STF_SUBTYPE,
  STF_MEMORY_ADDRESS,
} SymbolTableField;

typedef enum {
  SST_NONE,
  SST_INTEGER,
  SST_BOOLEAN
} SymbolSubType;

struct _SymbolTable {
  GNode	*root;
  GNode	*current_level;
};

struct _Symbol {
  gchar		*name;
  SymbolType	type;
  SymbolSubType subtype;
  gint		memory_address;
};

SymbolTable	*symbol_table_new(void);
void		symbol_table_free(SymbolTable *st);

Symbol		*symbol_table_install(SymbolTable *st, gchar *name, SymbolType type, SymbolSubType subtype);

gboolean	symbol_table_is_defined(SymbolTable *st, gchar *name, gint level);
gint		symbol_table_get_context_level(SymbolTable *st);
gboolean	symbol_table_is_installed(SymbolTable *st, gchar *name);
Symbol		*symbol_table_lookup_symbol(SymbolTable *st, gchar *name);

void		symbol_table_set_attribute_int(SymbolTable *st, gchar *name, SymbolTableField field, gint value);
gint		symbol_table_get_attribute_int(SymbolTable *st, gchar *name, SymbolTableField field);

void		symbol_table_context_enter(SymbolTable *st, gchar *context);
void		symbol_table_context_leave(SymbolTable *st);
void		symbol_table_context_reset(SymbolTable *st);

void		symbol_table_print(SymbolTable *st);

int		symbol_table_test_main(int argc, char **argv);

#endif /* __SYMBOL_TABLE_H__ */
