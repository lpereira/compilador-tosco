#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"

gboolean
codegen_traverse_func(GNode *node, gpointer data)
{
	if (node->parent) {
		ASTNode        *ast_node = (ASTNode *) node->data, *ast_parent = (ASTNode *) node->parent->data;

		switch(ast_node->token) {
			case T_WRITE:
				printf("WRITE %s\n", (gchar*)ast_node->data);
				break;
			case T_READ:
				printf("READ %s\n", (gchar*)ast_node->data);
				break;
			default:
				break;
		}

/*		printf("\"%s %s (%p)\" -> \"%s %s (%p)\";\n",
		       literals[ast_parent->token], (char *) ast_parent->data, node->parent,
		  literals[ast_node->token], (char *) ast_node->data, node);
*/
	}
	
	return FALSE;
}


void
codegen(GNode *ast)
{
	g_node_traverse(ast, G_PRE_ORDER, G_TRAVERSE_ALL, -1, codegen_traverse_func, NULL);
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