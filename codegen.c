#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"

	
static guint
label_new(void)
{
	static guint label = 0;
	
	return ++label;
}

static guint __temp_value = 0;
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

static void
codegen_children(GNode *nodes)
{
	GNode *n;

	for (n = nodes->children; n; n = n->next) {
		codegen(n);
	}	
}

static void
codegen_var(GNode *types)
{
	GNode *type;
	gint size;
	
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
		
		g_print("alloc %d\n", size * g_node_n_children(type));
	}
}

static void
codegen_if(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
	
	l1 = label_new();
	l2 = label_new();

	r = codegen(nodes->children);
	g_print("if_not t%d goto label%d\n", r, l1);
	temp_reset();

	for (n = nodes->children->next; n; n = n->next) {
		ASTNode  *ast_node = (ASTNode *) n->data;
		
		if (ast_node->token != T_ELSE) {
			codegen(n);
		} else {
			g_print("goto label%d\n", l2);
			g_print("\nlabel%d:\n", l1);
			codegen_children(n);
		}
	}
	
	g_print("\nlabel%d:\n", l2);
}

static void
codegen_while(GNode *nodes)
{
	GNode *n;
	guint r, l1, l2;
		
	l1 = label_new();
	l2 = label_new();
	
	g_print("\nlabel%d:\n", l1);
	r = codegen(nodes->children);
	g_print("if_not t%d goto label%d\n", r, l2);
	temp_reset();
	
	for (n = nodes->children->next; n; n = n->next) {
		codegen(n);
	}
	
	g_print("goto label%d\n", l1);
	g_print("\nlabel%d:\n", l2);

}

guint
codegen(GNode *node)
{
	guint r, t1, t2;
	ASTNode  *ast_node = (ASTNode *) node->data;
		
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
			t1 = codegen(node->children);
			t2 = codegen(node->children->next);
			
			r = temp_new();
			g_print("t%d := t%d %s t%d\n", r, t1, literals[ast_node->token], t2);

			temp_reset();
			temp_reset();
		
			break;
		
		case T_NUMBER:
			r = temp_new();
			g_print("t%d := %s\n", r, (gchar *)ast_node->data);
			
			break;
		
		case T_ATTRIB:			
		 	r = codegen(node->children);
			g_print("store %s, t%d\n", (gchar *)ast_node->data, r);			
			temp_reset();
			break;
		
		case T_VAR:
			codegen_var(node);
			break;
		
		case T_IDENTIFIER:
			r = temp_new();
			g_print("t%d := %s\n", r, (gchar *)ast_node->data);
			
			break;

		case T_READ:
			g_print("read_integer %s\n", (gchar *)ast_node->data);
			break;
		
		case T_WRITE:
			g_print("write_integer %s\n", (gchar *)ast_node->data);
			break;
		
		case T_WHILE:
			codegen_while(node);
			break;
		
		case T_IF:
			codegen_if(node);
			break;

		case T_PROGRAM:
			codegen_children(node);
			break;
		
		default:
			g_print("; %s %s\n", literals[ast_node->token], (gchar *)ast_node->data);
	}
	
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