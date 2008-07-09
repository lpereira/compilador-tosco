/*
 * Simple Pascal Compiler
 * Code Generator
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
/*
 * BUGS / ISSUES / TODO / FIXME / WTFBBQ
 *  - the code crashes sometimes; symbol table issues? (SEEMS FIXED?)
 *  - function and procedure calling code generation is wrong (SEEMS FIXED?)
 *  - must add a jump to the "main program" right after global variable
 *    declaration
 *  - we're printing out the code; this isn't a good idea to generate
 *    the final code later -- include the generated code in a list,
 *    and use sane structures
 *  - temp_new/temp_reset. is this sane?
 *  - unary operations are not supported (support must come right from the lex)
 *  - neither are T_TRUE and T_FALSE values
 *  - when a function returns, the allocated variables must be freed
 *    before "return"
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"
#include "symbol-table.h"
#include "stack.h"

/***/

static guint __temp_value = 0, __label_value = 0;
static SymbolTable *symbol_table = NULL;
static Stack *context = NULL;

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
temp_reset(void)
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
			default:		/* assume size 4 */
				size = 4;
				kind = SK_INTEGER;
		}

		for (var = type->children; var; var = var->next) {
			ast_node = (ASTNode *)var->data;

			symbol_table_install(symbol_table, (gchar *)ast_node->data, ST_VARIABLE, kind);
			symbol_table_set_size_and_offset(symbol_table, (gchar *)ast_node->data, size, total_size);

			total_size += size;
		}
	}

	printf("alloc %d\n", total_size);
	
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
	printf("if_not t%d goto label%d\n", r, l1);
	temp_reset();

	for (n = nodes->children->next; n; n = n->next) {
		ASTNode *ast_node = (ASTNode *) n->data;
		
		if (ast_node->token != T_ELSE) {
			generate(n);
		} else {
			printf("goto label%d\n", l2);
			printf("\nlabel%d:\n", l1);
			generate_children(n);
			
			has_else = TRUE;
		}
	}
	
	if (has_else)
		printf("\nlabel%d:\n", l2);
	else
		printf("\nlabel%d:\n", l1);

	return 0;
}

static guint
generate_while(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
		
	l1 = label_new();
	l2 = label_new();
	
	printf("\nlabel%d:\n", l1);
	r = generate(nodes->children);
	printf("if_not t%d goto label%d\n", r, l2);
	temp_reset();
	
	for (n = nodes->children->next; n; n = n->next) {
		generate(n);
	}
	
	printf("goto label%d\n", l1);
	printf("\nlabel%d:\n", l2);

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
	
	printf("t%d := t%d %s t%d\n", r, t1, literals[ast_node->token], t2);

	temp_reset();
	temp_reset();
	
	return r;
}

static guint
generate_number(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r;
	
	r = temp_new();
	printf("t%d := %s\n", r, (gchar *)ast_node->data);
	
	return r;
}

static guint
generate_number(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	GNode *children = node->children;
	guint r, offset, size;
	
	r = generate(children);

	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	printf("store stack_pointer-%d, size %d, t%d /* %s */\n", offset, size, r, (gchar *)ast_node->data);
	temp_reset();
	
	return r;	
}

static guint
generate_identifier(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint r, offset, size;
	
	r = temp_new();
	symbol_table_get_size_and_offset(symbol_table, (gchar *)ast_node->data, &size, &offset);
	printf("load t%d, stack_pointer-%d, size %d /* %s */\n", r, offset, size, (gchar *)ast_node->data);
	
	return r;
}

static guint
generate_read(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;

	printf("read %s\n", (gchar*) ast_node->data);

	return 0;
}

static guint
generate_write(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;

	printf("write %s\n", (gchar *) ast_node->data);

	return 0;
}

static guint
generate_procedure_or_function(GNode *node, guint procedure)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint var_size = 0, r, l;
	GNode *n;
	
	r = label_new();
	l = label_new();
	temp_zero();
	
	printf("goto label%d\n", l);
	
	if (procedure) {
		printf("\nlabel%d: /* procedure %s */\n", r, (gchar *)ast_node->data);
		symbol_table_install(symbol_table, (gchar *)ast_node->data, ST_PROCEDURE, SK_NONE);
	} else {
		printf("\nlabel%d: /* function %s */\n", r, (gchar *)ast_node->data);
		symbol_table_install(symbol_table, (gchar *)ast_node->data, ST_FUNCTION, SK_NONE);
	}

	symbol_table_set_label_number(symbol_table, (gchar *)ast_node->data, r);
	symbol_table_push_context(symbol_table);
	
	for (n = node->children; n; n = n->next) {
		ast_node = (ASTNode *)n->data;
		
		if (ast_node->token == T_VAR)
			var_size += generate_var(n);
		else
			generate(n);
	}
	
	if (var_size)
		printf("free %d\n", var_size);

	if (procedure)
		printf("return\n\n");

	symbol_table_pop_context(symbol_table);
	
	printf("label%d:\n", l);

	return r;	
}

static guint
generate_procedure(GNode *node)
{
	return generate_procedure_or_function(node, TRUE);
}

static void
context_save(void)
{
	guint i;
	
	stack_push(context, GINT_TO_POINTER(__temp_value));
	for (i = 1; i <= __temp_value; i++) {
		printf("push t%d\n", i);
	}
}

static void
context_restore(void)
{
	guint i;
	
	i = GPOINTER_TO_INT(stack_pop(context));
	for (; i >= 1; i--) {
		printf("pop t%d\n", i);
	}
}

static guint
generate_procedure_call(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint label;
	
	label = symbol_table_get_label_number(symbol_table, (gchar *)ast_node->data);
	
	context_save();
	printf("call label%d\n", label);
	context_restore();

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
	context_save();
	printf("call label%d\n", label);
	context_restore();

	printf("t%d := tr\n", r);
	
	return r;
}

static guint
generate_function_return(GNode *node)
{
	guint r;
	
	r = generate(node->children);
	printf("tr := t%d\n", r);
	printf("return\n\n");
	temp_reset();
	
	return 0;
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
			g_error("Houston, we have a problem! Don't know how to generate code for node type ``%s''.", literals[ast_node->token]);
	}
	
	return r;
}

guint
codegen(GNode *root)
{
	guint r;
	
	__temp_value = __label_value = 0;
	symbol_table = symbol_table_new();
	context = stack_new();
	
	r = generate(root);
	
	symbol_table_free(symbol_table);
	stack_free(context);
	
	return r;
}

#ifdef CODEGEN_TEST
int
main(int argc, char **argv)
{
	GNode          *root;
	TokenList      *token_list;
	
	if (argc >= 2) {
		fclose(stdin);
		stdin = fopen(argv[1], "r");		
	}
	
	token_list = lex();
	root = ast(token_list);
	tl_destroy(token_list);

	codegen(root);
	
	return 0;
}
#endif	/* CODEGEN_TEST */