#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"
#include "symbol-table.h"

/***/

static guint __temp_value = 0, __label_value = 0;
static SymbolTable *symbol_table;

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
	GNode *type;
	gint size, total_size = 0;
	
	for (type = types->children; type; type = type->next) {
		ASTNode *ast_node = (ASTNode *)type->data;
		
		switch (ast_node->token) {
			case T_BOOLEAN:
				size = 1;
				break;
			case T_INTEGER:
			default:		/* assume size 4 */
				size = 4;
		}
		
		size *= g_node_n_children(type);
		total_size += size;
		
		g_print("alloc %d\n", size);
	}
	
	return total_size;
}

static guint
generate_if(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
	
	l1 = label_new();
	l2 = label_new();

	r = generate(nodes->children);
	g_print("if_not t%d goto label%d\n", r, l1);
	temp_reset();

	for (n = nodes->children->next; n; n = n->next) {
		ASTNode  *ast_node = (ASTNode *) n->data;
		
		if (ast_node->token != T_ELSE) {
			generate(n);
		} else {
			g_print("goto label%d\n", l2);
			g_print("\nlabel%d:\n", l1);
			generate_children(n);
		}
	}
	
	g_print("\nlabel%d:\n", l2);

	return 0;
}

static guint
generate_while(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
		
	l1 = label_new();
	l2 = label_new();
	
	g_print("\nlabel%d:\n", l1);
	r = generate(nodes->children);
	g_print("if_not t%d goto label%d\n", r, l2);
	temp_reset();
	
	for (n = nodes->children->next; n; n = n->next) {
		generate(n);
	}
	
	g_print("goto label%d\n", l1);
	g_print("\nlabel%d:\n", l2);

	return 0;
}

static guint
generate_binop(GNode *node, gchar *op)
{
	guint r, t1, t2;
	
	t1 = generate(node->children);
	t2 = generate(node->children->next);
	
	r = temp_new();
	g_print("t%d := t%d %s t%d\n", r, t1, op, t2);

	temp_reset();
	temp_reset();
	
	return r;
}

static guint
generate_number(gchar *n)
{
	guint r;
	
	r = temp_new();
	g_print("t%d := %s\n", r, n);
	
	return r;
}

static guint
generate_attrib(GNode *children, gchar *var_name)
{
	guint r;
	
	r = generate(children);
	g_print("store %s, t%d\n", var_name, r);			
	temp_reset();
	
	return r;	
}

static guint
generate_identifier(gchar *var_name)
{
	guint r;
	
	r = temp_new();
	g_print("load t%d, %s\n", r, var_name);
	
	return r;
}

static guint
generate_read(gchar *var_name)
{
	g_print("read %s\n", var_name);	
	
	return 0;
}

static guint
generate_write(gchar *var_name)
{
	g_print("write %s\n", var_name);
	
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
	
	g_print("goto label%d\n", l);
	
	if (procedure)
		g_print("\nlabel%d: /* procedure %s */\n", r, (gchar *)ast_node->data);
	else
		g_print("\nlabel%d: /* function %s */\n", r, (gchar *)ast_node->data);
	
	symbol_table_install(symbol_table, (gchar *)ast_node->data, ST_PROCEDURE, r);
	symbol_table_push_context(symbol_table);
	
	for (n = node->children; n; n = n->next) {
		ast_node = (ASTNode *)n->data;
		
		if (ast_node->token == T_VAR)
			var_size = generate_var(n);
		else
			generate(n);
	}
	
	if (var_size)
		g_print("free %d\n", var_size);

	if (procedure)
		g_print("return\n\n");
	symbol_table_pop_context(symbol_table);
	
	g_print("label%d:\n", l);

	return r;	
}

static guint
generate_procedure(GNode *node)
{
	return generate_procedure_or_function(node, TRUE);
}

static guint
generate_procedure_call(GNode *node)
{
	ASTNode *ast_node = (ASTNode *)node->data;
	guint label;
	
	label = symbol_table_get_entry_kind(symbol_table, (gchar *)ast_node->data);
	g_print("call_proc label%d\n", label);

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
	
	r = temp_new();
	label = symbol_table_get_entry_kind(symbol_table, (gchar *)ast_node->data);
	
	g_print("t%d := call_fun label%d\n", r, label);
	
	return r;
}

static guint
generate_function_return(GNode *node)
{
	guint r;
	
	r = generate(node->children);
	g_print("return_value t%d\n\n", r);
	temp_reset();
	
	return 0;
}

static guint
generate(GNode *node)
{
	guint r = 0;
	ASTNode *ast_node = (ASTNode *) node->data;
		
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
			r = generate_binop(node, (gchar *)literals[ast_node->token]);
			break;
		
		case T_NUMBER:
			r = generate_number((gchar *)ast_node->data);			
			break;
		
		case T_ATTRIB:
			r = generate_attrib(node->children, (gchar *)ast_node->data);
			break;
		
		case T_VAR:
			r = generate_var(node);
			break;
		
		case T_IDENTIFIER:
			r = generate_identifier((gchar *)ast_node->data);
			break;

		case T_READ:
			r = generate_read((gchar *)ast_node->data);
			break;
		
		case T_WRITE:
			r = generate_write((gchar *)ast_node->data);
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
			g_error("Panic! Don't know how to generate code for node ``%s''.", literals[ast_node->token]);
	}
	
	return r;
}

guint
codegen(GNode *root)
{
	guint r;
	
	__temp_value = __label_value = 0;
	symbol_table = symbol_table_new();
	
	r = generate(root);
	
	symbol_table_free(symbol_table);
	
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