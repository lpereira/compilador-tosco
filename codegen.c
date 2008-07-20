/*
 * Simple Pascal Compiler
 * Code Generator
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
/*
 * BUGS / ISSUES / TODO / FIXME / WTFBBQ
 *  - the code crashes sometimes; symbol table issues? (SEEMS FIXED?)
 *  - must add a jump to the "main program" right after global variable
 *    declaration (we currently do a jump to a label, which jumps ahead until
 *    we find the main program; this works but is kludgy)
 *  - we're printing out the code; this isn't a good idea to generate
 *    the final code later -- include the generated code in a list,
 *    and use sane structures
 *  - temp_new/temp_unref. is this sane?
 *  - free global variables memory
 *  - mp will not work on recursive functions!
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"
#include "codeemitter.h"
#include "symbol-table.h"
#include "stack.h"

/***/

static guint __temp_value = 0, __label_value = 0;
static SymbolTable *symbol_table = NULL;
static Stack *context = NULL, *var_sizes = NULL;
static Emitter *emitter = NULL;

/***/

static guint generate(GNode *node);
	
/***/

static guint
label_new(void)
{
	return ++__label_value;
}

static guint
temp_new(void)
{
	return ++__temp_value;
}

static void
temp_zero(void)
{
	__temp_value = 0;
}

static void
temp_unref(void)
{
	__temp_value--;
}

static guint
generate_children(GNode *nodes)
{
	GNode *n;

	for (n = nodes->children; n; n = n->next) {
		generate(n);
	}
	
	return 0;
}

static guint
generate_var(GNode *types)
{
	GNode *type, *var;
	gint size, total_size = 0;
	STEntryKind kind;
	
	for (type = types->children; type; type = type->next) {
		ASTNode *ast_node = (ASTNode *)type->data;
		
		switch (ast_node->token) {
			case T_BOOLEAN:
				size = 1;
				kind = SK_BOOLEAN;
				break;
			case T_INTEGER:
			default:		/* assume size 2 */
				size = 2;
				kind = SK_INTEGER;
		}

		for (var = type->children; var; var = var->next) {
			ast_node = (ASTNode *)var->data;

			symbol_table_install(symbol_table, (gchar *)ast_node->data, ST_VARIABLE, kind);
			symbol_table_set_size_and_offset(symbol_table, (gchar *)ast_node->data, size, total_size);

			total_size += size;
		}
	}

	emitter_emit_alloc(emitter, total_size);
	
	return total_size;
}

static guint
generate_if(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
	gboolean has_else = FALSE;
	
	l1 = label_new();
	l2 = label_new();

	r = generate(nodes->children);
	emitter_emit_ifnot(emitter, r, l1);
	temp_unref();

	for (n = nodes->children->next; n; n = n->next) {
		ASTNode *ast_node = (ASTNode *) n->data;
		
		if (ast_node->token != T_ELSE) {
			generate(n);
		} else {
			emitter_emit_goto(emitter, l2);
			emitter_emit_label(emitter, l1);
			generate_children(n);
			
			has_else = TRUE;
		}
	}
	
	if (has_else) {
		emitter_emit_label(emitter, l2);
	} else {
		emitter_emit_label(emitter, l1);
	}

	return 0;
}

static guint
generate_while(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
		
	l1 = label_new();
	l2 = label_new();
	
	emitter_emit_label(emitter, l1);
	r = generate(nodes->children);
	emitter_emit_ifnot(emitter, r, l2);
	temp_unref();
	
	for (n = nodes->children->next; n; n = n->next) {
		generate(n);
	}
	
	emitter_emit_goto(emitter, l1);
	emitter_emit_label(emitter, l2);

	return 0;
}

static guint
generate_binop(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, t1, t2;
	
	r = temp_new();

	t1 = generate(node->children);
	t2 = generate(node->children->next);
	
	emitter_emit_binop(emitter, r, t1, literals[ast_node->token], t2);

	temp_unref();
	temp_unref();
	
	return r;
}

static guint
generate_number(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r;
	
	r = temp_new();
	
	emitter_emit_ldimed(emitter, r, atoi((gchar *)ast_node->data));

	return r;
}

static guint
generate_attrib(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	GNode *children = node->children;
	guint r, offset, size;
	
	r = generate(children);

	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	offset += symbol_table_get_current_offset(symbol_table, (gchar *)ast_node->data);
	
	emitter_emit_store(emitter, r, offset, size);

	temp_unref();

	return r;	
}

static guint
generate_identifier(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, offset, size;

	r = temp_new();

	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	offset += symbol_table_get_current_offset(symbol_table, (gchar *)ast_node->data);
	
	emitter_emit_load(emitter, r, offset, size);

	return r;
}

static guint
generate_read(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, offset, size;
	
	r = temp_new();
	emitter_emit_read(emitter, r);

	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	offset += symbol_table_get_current_offset(symbol_table, (gchar *)ast_node->data);

	emitter_emit_store(emitter, r, offset, size);
	temp_unref();

	return r;
}

static guint
generate_write(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, offset, size;
	
	r = temp_new();
	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	offset += symbol_table_get_current_offset(symbol_table, (gchar *)ast_node->data);

	emitter_emit_load(emitter, r, offset, size);
	emitter_emit_write(emitter, r);

	temp_unref();

	return r;
}

static guint
generate_function_return(GNode *node)
{
	guint r, var_size;
	
	r = generate(node->children);
	emitter_emit_return_value(emitter, r);
	
	if ((var_size = GPOINTER_TO_UINT(stack_peek(var_sizes)))) {
		emitter_emit_free(emitter, var_size);
	}
	
	emitter_emit_return(emitter);
	temp_unref();
	
	return 0;
}

static guint
generate_procedure_or_function(GNode *node, guint procedure)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint var_size = 0, l, r;
	GNode *n;
	
	r = label_new();
	l = label_new();
	temp_zero();
	
	emitter_emit_goto(emitter, l);
	emitter_emit_label(emitter, r);
	
	symbol_table_install(symbol_table, (gchar *)ast_node->data,
						 procedure ? ST_PROCEDURE : ST_FUNCTION, SK_NONE);
	symbol_table_set_label_number(symbol_table, (gchar *)ast_node->data, r);
	symbol_table_push_context(symbol_table);
	
	if (!procedure) {
		stack_push(var_sizes, GUINT_TO_POINTER(var_size));
	}

	for (n = node->children; n; n = n->next) {
		ast_node = (ASTNode *)n->data;
		
		if (ast_node->token == T_VAR) {
			var_size += generate_var(n);
			
			if (!procedure) {
				stack_pop(var_sizes);
				stack_push(var_sizes, GUINT_TO_POINTER(var_size));
			}
		} else {
			generate(n);
		}
	}
	
	if (var_size && ast_node->token != T_FUNCTION_RETURN) {
		emitter_emit_free(emitter, var_size);
	}

	if (procedure) {
		emitter_emit_return(emitter);
	} else {
		stack_pop(var_sizes);
	}
	
	emitter_emit_label(emitter, l);

	symbol_table_pop_context(symbol_table);

	return 0;
}

static guint
generate_procedure(GNode *node)
{
	return generate_procedure_or_function(node, TRUE);
}

static void
context_save(guint except)
{
	guint i;

	stack_push(context, GINT_TO_POINTER(__temp_value));
	for (i = 1; i <= __temp_value; i++) {
		if (i != except)
			emitter_emit_pushreg(emitter, i);
	}
}

static void
context_restore(guint except)
{
	guint i;

	i = GPOINTER_TO_INT(stack_pop(context));
	for (; i >= 1; i--) {
		if (i != except)
			emitter_emit_popreg(emitter, i);
	}
}

static guint
generate_procedure_call(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint label;

	label = symbol_table_get_label_number(symbol_table, (gchar *)ast_node->data);

	context_save(-1);
	emitter_emit_pcall(emitter, label);
	context_restore(-1);

	return 0;
}

static guint
generate_function(GNode *node)
{
	return generate_procedure_or_function(node, FALSE);
}

static guint
generate_function_call(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, label;

	label = symbol_table_get_label_number(symbol_table, (gchar *)ast_node->data);

	r = temp_new();
	context_save(r);
	emitter_emit_fcall(emitter, label);
	context_restore(r);

	emitter_emit_copy_return_value(emitter, r);
	
	return r;
}

static guint
generate_true(GNode *node)
{
	guint r;
	
	r = temp_new();
	emitter_emit_ldimed(emitter, r, 1);
	
	return r;
}

static guint
generate_false(GNode *node)
{
	guint r;
	
	r = temp_new();
	emitter_emit_ldimed(emitter, r, 0);
	
	return r;
}

static guint
generate_not(GNode *node)
{
	guint r;
	
	r = generate(node->children);
	emitter_emit_unop(emitter, r, "not", r);
	
	return r;
}

static guint
generate_uminus(GNode *node)
{
	guint r;
	
	r = generate(node->children);
	emitter_emit_unop(emitter, r, "neg", r);
	
	return r;
}

static guint
generate(GNode *node)
{
	ASTNode *ast_node = (ASTNode *) node->data;
	guint r = 0;
		
	switch (ast_node->token) {
		case T_MINUS:
		case T_PLUS:
		case T_DIVIDE:
		case T_MULTIPLY:
		case T_OP_EQUAL:
		case T_OP_GT:
		case T_OP_GEQ:
		case T_OP_LT:
		case T_OP_LEQ:
		case T_OP_DIFFERENT:
			r = generate_binop(node);
			break;
		case T_NOT:
			r = generate_not(node);
			break;
		case T_UNARY_MINUS:
			r = generate_uminus(node);
			break;
		case T_TRUE:
			r = generate_true(node);
			break;
		case T_FALSE:
			r = generate_false(node);
			break;
		case T_NUMBER:
			r = generate_number(node);
			break;
		case T_ATTRIB:
			r = generate_attrib(node);
			break;
		case T_VAR:
			r = generate_var(node);
			break;
		case T_IDENTIFIER:
			r = generate_identifier(node);
			break;
		case T_READ:
			r = generate_read(node);
			break;
		case T_WRITE:
			r = generate_write(node);
			break;
		case T_WHILE:
			r = generate_while(node);
			break;
		case T_IF:
			r = generate_if(node);
			break;
		case T_PROGRAM:
			r = generate_children(node);
			break;
		case T_PROCEDURE:
			r = generate_procedure(node);
			break;
		case T_PROCEDURE_CALL:
			r = generate_procedure_call(node);
			break;
		case T_FUNCTION:
			r = generate_function(node);
			break;
		case T_FUNCTION_CALL:
			r = generate_function_call(node);
			break;
		case T_FUNCTION_RETURN:
			r = generate_function_return(node);
			break;
		default:
			g_error("Houston, we have a problem! Don't know how to "
			        "generate code for node type ``%s'' (%d).",
			        literals[ast_node->token], ast_node->token);
	}
	
	return r;
}

Emitter *
codegen(GNode *root)
{
	__temp_value = __label_value = 0;
	
	emitter = emitter_new();
	symbol_table = symbol_table_new();
	context = stack_new();
	var_sizes = stack_new();
	
	generate(root);
	
	symbol_table_free(symbol_table);
	stack_free(context);
	stack_free(var_sizes);
	
	return emitter;
}

int
codegen_test_main(int argc, char **argv)
{
	GNode          *root;
	TokenList      *token_list;

	token_list = lex();
	root = ast(token_list);

	codegen(root);

	tl_destroy(token_list);

	return 0;
}
