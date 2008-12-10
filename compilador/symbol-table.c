/*
 * Simple Pascal Compiler
 * Symbol Table
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 */
#include <glib.h>
#include <string.h>

#include "lex.h"
#include "ast.h"
#include "symbol-table.h"

const gchar *symbol_types[]    = { "nenhum", "variável", "função", "procedimento", "programa" };
const gchar *symbol_subtypes[] = { "nenhum", "inteiro", "booleano" };

SymbolTable *
symbol_table_new(void)
{
  SymbolTable *st;
  
  st = g_new0(SymbolTable, 1);
  st->root = NULL;
  st->current_level = NULL;
  
  return st;
}

void
symbol_table_free(SymbolTable *st)
{
  symbol_table_print(st);
}

Symbol *
symbol_table_install(SymbolTable *st, gchar *name, SymbolType type, SymbolSubType subtype)
{
  GNode *node;
  Symbol *symbol;
  
  symbol = g_new0(Symbol, 1);
  symbol->name = g_strdup(name);
  symbol->type = type;
  symbol->subtype = subtype;
  
  node = g_node_new(symbol);
  
  if (!st->root) {
    st->root = st->current_level = node;
  } else {
    g_node_append(st->current_level, node);
  }
  
  return symbol;
}

static GNode *
symbol_table_get_node_at_context(SymbolTable *st, GNode *context, gchar *name)
{
  GNode *node;
  Symbol *symbol;
  
  for (node = context->children; node; node = node->next) {
    symbol = (Symbol *)node->data;
    
    if (g_str_equal(symbol->name, name)) {
      return node;
    }
  }
  
  return NULL;
}

static GNode *
symbol_table_get_node_at_current_context(SymbolTable *st, gchar *name)
{
  return symbol_table_get_node_at_context(st, st->current_level, name);
}

gboolean
symbol_table_is_defined(SymbolTable *st, gchar *name, gint level)
{
  GNode *l;
  
  for (l = st->current_level; l && level--; l = l->parent) {
    if (symbol_table_get_node_at_context(st, l, name)) {
        return TRUE;
    }
  }
  
  return FALSE;
}

gboolean
symbol_table_is_installed(SymbolTable *st, gchar *name)
{
  return symbol_table_lookup_symbol(st, name) != NULL;
}

Symbol *
symbol_table_lookup_symbol(SymbolTable *st, gchar *name)
{
  GNode *level, *node;

  for (level = st->current_level; level; level = level->parent) { 
    if ((node = symbol_table_get_node_at_context(st, level, name))) {
       return (Symbol *)node->data;
    }
  }

  return NULL;
}

void
symbol_table_set_attribute_int(SymbolTable *st, gchar *name, SymbolTableField field, gint value)
{
  Symbol *symbol;

  if ((symbol = symbol_table_lookup_symbol(st, name))) {
    switch (field) {
    case STF_TYPE:
      symbol->type = value; break;
    case STF_SUBTYPE:
      symbol->subtype = value; break;
    case STF_MEMORY_ADDRESS:
      symbol->memory_address = value; break;
    default: return;
    }
  }
}

gint
symbol_table_get_attribute_int(SymbolTable *st, gchar *name, SymbolTableField field)
{
  Symbol *symbol;

  if ((symbol = symbol_table_lookup_symbol(st, name))) {
    switch (field) {
    case STF_TYPE:
      return symbol->type;
    case STF_SUBTYPE:
      return symbol->subtype;
    case STF_MEMORY_ADDRESS:
      return symbol->memory_address;
    default: return 0;
    }
  }

  return 0;
}

void
symbol_table_context_enter(SymbolTable *st, gchar *context)
{
  GNode *node;
  
  if (!st->current_level) {
    return;
  }
  
  if ((node = symbol_table_get_node_at_current_context(st, context))) {
    st->current_level = node;
  }
}

void
symbol_table_context_leave(SymbolTable *st)
{
  st->current_level = st->current_level->parent;
}

void
symbol_table_context_reset(SymbolTable *st)
{
  st->current_level = st->root;
}

gint
symbol_table_get_context_level(SymbolTable *st)
{
  return g_node_depth(st->current_level);
}

static void
symbol_table_print_func(SymbolTable *st, GNode *node)
{
  Symbol *symbol = (Symbol *)node->data;
  guint depth = g_node_depth(node);
  gchar indentation[512];

  memset(indentation, ' ', depth - 1);
  indentation[depth - 1] = '\0';
  
  printf("%s%s|%s|%s|%d\n", indentation, symbol->name,
         symbol_types[symbol->type], symbol_subtypes[symbol->subtype],
	 symbol->memory_address);
  
  for (node = node->children; node; node = node->next) {
    symbol_table_print_func(st, node);
  }
}

void
symbol_table_print(SymbolTable *st)
{
  symbol_table_print_func(st, st->root);
}

int
symbol_table_test_main(int argc, char **argv)
{
    GNode *root;
    TokenList *token_list;

    token_list = lex();
    root = ast(token_list);

    tl_unref(token_list);

    symbol_table_print(symbol_table);

    return 0;
}

