#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "codegen.h"

static guint
new_label(void)
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
	//__temp_value--;
}



guint
codegen(GNode *node)
{
	guint r, t1, t2;
	gchar *op;
	
	ASTNode  *ast_node = (ASTNode *) node->data;
	
	switch (ast_node->token) {
		case T_MINUS:
			op = "-";
		case T_PLUS:
			op = "+";
		
			t1 = codegen(node->children);
			t2 = codegen(node->children->next);
			
			r = temp_new();
			g_print("t%d := t%d %s t%d\n", r, t1, op, t2);
			temp_reset();
		
			break;
		
		case T_NUMBER:
			r = temp_new();
			g_print("t%d := %s\n", r, (gchar *)ast_node->data);
			temp_reset();
			
			break;
		
		case T_ATTRIB:			
		 	r = codegen(node->children);
			
			g_print("store $%s, t%d\n", (gchar *)ast_node->data, r);
			
			temp_reset();
			break;
		
		case T_VAR:
			
			break;
		
		case T_IDENTIFIER:
			r = temp_new();
			g_print("t%d := load %s\n", r, (gchar *)ast_node->data);
			temp_reset();
			
			break;
		
		case T_PROGRAM:
			{
				GNode *n;

				for (n = node->children; n; n = n->next) {
					codegen(n);
				}
			}
			break;
		
		default:
			g_print("/* %s %s */\n", literals[ast_node->token], (gchar *)ast_node->data);
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