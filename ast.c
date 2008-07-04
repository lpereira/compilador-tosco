/*
 * Simple Pascal Compiler
 * Abstract Syntax Tree
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@linuxmag.com.br>
 *
 * ast_expression() is based on Exp_to_Tree (Function to Convert Infix Expression to Expression Tree),
 * written by Michael B. Feldman <mfeldman@gwu.edu>.
 */
/*
 * TODO: - nome_de_funcao := retorno tem que ser validado (nao eh possivel retornar de uma funcao que nao a atual!)
 *         tambem nao deve ser possivel retornar valor de procedimento
 */

#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h>

#include <glib.h>

#include "lex.h"
#include "ast.h"
#include "stack.h"
#include "symbol-table.h"

void            __ast_recursive(GNode * root, GList ** token_list, TokenType end_with);
void inline     ast_recursive(GNode * root, GList ** token_list);
void inline     ast_recursive_stmt(GNode * root, GList ** token_list);

static SymbolTable *symbol_table;

static void
ast_error_token(Token *token, gchar *message)
{
	g_print("Error: line %d, column %d, token ``%s'': %s\n",
			token->line, token->column, token->id, message);
	exit(1);
}

ASTNode        *
ast_node_new(TokenType token, gpointer data)
{
	ASTNode        *node;

	node = g_new0(ASTNode, 1);
	node->token = token;
	node->data = data;

	return node;
}

void
ast_var(GNode * root, GList ** tokens)
{
	GNode          *var_root, *var_type_root;
	GList          *vars = NULL, *v;
	Token          *token;
	STEntryKind		kind;

	var_root = g_node_new(ast_node_new(T_VAR, NULL));
	g_node_append(root, var_root);

	for (*tokens = (*tokens)->next; *tokens; *tokens = (*tokens)->next) {
		token = (Token *) (*tokens)->data;

		switch (token->type) {
		case T_IDENTIFIER:
			vars = g_list_prepend(vars, token);
			break;
		case T_INTEGER:
		case T_BOOLEAN:
			kind = (token->type == T_INTEGER) ? SK_INTEGER : SK_BOOLEAN;
			var_type_root = g_node_new(ast_node_new(token->type, NULL));
			
			g_node_append(var_root, var_type_root);

			for (v = vars; v; v = v->next) {
				token = (Token *)v->data;
				
				if (!symbol_table_is_installed(symbol_table, token->id)) {
					ASTNode *ast_node = ast_node_new(token->type, token->id);

					g_node_append(var_type_root, g_node_new(ast_node));
					symbol_table_install(symbol_table, token->id, ST_VARIABLE, kind);
				} else {
					ast_error_token(token, "symbol already defined");
				}
			}

			g_list_free(vars);
			vars = NULL;
			break;
		default:
			/*
			 * nao vamos fazer nada com esse token, devolve-o
			 * para a lista
			 */
			*tokens = g_list_previous(*tokens);
			return;
		}
	}
}

void
ast_function(GNode * root, GList ** tokens)
{
	GNode          *func_node;
	Token          *token;
	GList 		   *t;
	
	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	func_node = g_node_new(ast_node_new(T_FUNCTION, token->id));
	g_node_append(root, func_node);

	symbol_table_install(symbol_table, token->id, ST_FUNCTION, 
						 ((Token *)(*tokens)->next->data)->type == T_INTEGER ? SK_INTEGER : SK_BOOLEAN);

	*tokens = (*tokens)->next;
	*tokens = (*tokens)->next;
	
	switch (((Token *)(*tokens)->data)->type) {
		case T_BEGIN:
			*tokens = (*tokens)->next;
			break;
		case T_VAR:
			for (t = *tokens; t; t = t->next) {
				token = (Token *)t->data;
				
				if (token->type == T_BEGIN) {
					token->type = T_NONE;
					break;
				}
			}
		default:
			break;
	}

	symbol_table_push_context(symbol_table);
	ast_recursive(func_node, tokens);
	symbol_table_pop_context(symbol_table);
}

void
ast_procedure(GNode * root, GList ** tokens)
{
	GNode          *proc_node;
	Token          *token;
	GList 		   *t;
	
	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	proc_node = g_node_new(ast_node_new(T_PROCEDURE, token->id));
	g_node_append(root, proc_node);

	symbol_table_install(symbol_table, token->id, ST_PROCEDURE, SK_NONE);

	*tokens = (*tokens)->next;

	switch (((Token *)(*tokens)->data)->type) {
		default:
			break;
		case T_BEGIN:
			*tokens = (*tokens)->next;
			break;
		case T_VAR:
			for (t = *tokens; t; t = t->next) {
				token = (Token *)t->data;
				
				if (token->type == T_BEGIN) {
					token->type = T_NONE;
					break;
				}
			}
	}

	symbol_table_push_context(symbol_table);
	ast_recursive(proc_node, tokens);
	symbol_table_pop_context(symbol_table);
}

static int inline
__op_priority(TokenType op)
{
	switch (op) {
	default:
		break;
	case T_PLUS:
	case T_MINUS:
		return 0;
	case T_MULTIPLY:
	case T_DIVIDE:
		return 1;
	case T_NOT:
		return 2;
	case T_OP_DIFFERENT:
	case T_OP_EQUAL:
	case T_OP_GT:
	case T_OP_LT:
	case T_OP_GEQ:
	case T_OP_LEQ:
	case T_OR:
		return 3;
	}

	return 0;
}

TokenType inline
ast_node_token(GNode * node)
{
	ASTNode        *ast_node;

	return (node && ((ast_node = (ASTNode *) node->data))) ? ast_node->token : T_NONE;
}

static void 
pop_connect_push(Stack * op_stack, Stack * node_stack)
{
	GNode          *temp, *left, *right;

	temp  = (GNode *) stack_pop(op_stack);
	right = (GNode *) stack_pop(node_stack);
	left  = (GNode *) stack_pop(node_stack);

	g_node_append(temp, left);
	g_node_append(temp, right);

	stack_push(node_stack, temp);
}

void
ast_expression(GNode * root, GList ** tokens, TokenType stop)
{
	Token          *token;
	Stack          *op_stack, *node_stack;

	op_stack = stack_new();
	node_stack = stack_new();

	for (*tokens = (*tokens)->next; *tokens; *tokens = (*tokens)->next) {
		token = (Token *) (*tokens)->data;

		if (token->type == stop) {
			break;
		}
		
		switch (token->type) {
		default:
			break;
		case T_NUMBER:
			stack_push(node_stack, g_node_new(ast_node_new(token->type, token->id)));
			break;
		case T_IDENTIFIER:
			switch (symbol_table_get_entry_type(symbol_table, token->id)) {
				case ST_VARIABLE:
					/* FIXME: check if it is compatible with expression */
					break;
				case ST_FUNCTION:
					/* FIXME: check if function return type is compatible with expression */
					token->type = T_FUNCTION_CALL;
					break;
				case ST_NONE:
					ast_error_token(token, "undefined symbol");
					break;
				default:
					ast_error_token(token, "not a variable or function");
			}

			stack_push(node_stack, g_node_new(ast_node_new(token->type, token->id)));
			break;
		case T_PLUS:
		case T_MINUS:
		case T_MULTIPLY:
		case T_DIVIDE:
		case T_NOT:
		case T_OP_DIFFERENT:
		case T_OP_EQUAL:
		case T_OP_GT:
		case T_OP_LT:
		case T_OP_GEQ:
		case T_OP_LEQ:
		case T_OR:
			if (stack_is_empty(op_stack)) {
				stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
			} else if (ast_node_token(stack_peek(op_stack)) == T_OPENPAREN) {
				stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
			} else if (__op_priority(ast_node_token(stack_peek(op_stack))) < __op_priority(token->type)) {
				stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
			} else {
				do {
					pop_connect_push(op_stack, node_stack);
				} while (!(stack_is_empty(op_stack) ||
					   ast_node_token(stack_peek(op_stack)) == T_OPENPAREN ||
					   __op_priority(ast_node_token(stack_peek(op_stack))) < __op_priority(token->type)));

				stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
			}
			break;
		case T_OPENPAREN:
			stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
			break;
		case T_CLOSEPAREN:
			while (ast_node_token(stack_peek(op_stack)) != T_OPENPAREN) {
				pop_connect_push(op_stack, node_stack);
			}
			stack_pop(op_stack);
			break;
		}
	}

	while (!stack_is_empty(op_stack)) {
		pop_connect_push(op_stack, node_stack);
	}

	g_node_append(root, stack_pop(node_stack));

	stack_free(op_stack);
	stack_free(node_stack);
}

void
ast_attrib(GNode * root, GList ** tokens)
{
	Token          *token;
	GNode          *attrib_node;
	ASTNode	       *ast_node;

	token = (Token *) (*tokens)->prev->data;

	if (symbol_table_is_installed(symbol_table, token->id)) {
		switch (symbol_table_get_entry_type(symbol_table, token->id)) {
			case ST_VARIABLE:
				ast_node = ast_node_new(T_ATTRIB, token->id);
				break;
			case ST_FUNCTION:
				ast_node = ast_node_new(T_FUNCTION_RETURN, token->id);
				break;
			default:
				ast_error_token(token, "not a variable or function");
		}
		
		attrib_node = g_node_new(ast_node);
		g_node_append(root, attrib_node);

		ast_expression(attrib_node, tokens, T_SEMICOLON);
	} else {
		ast_error_token(token, "undefined symbol");
	}
}

void
ast_else(GNode * root, GList ** tokens)
{
	Token          *token;
	GNode          *else_node;

	else_node = g_node_new(ast_node_new(T_ELSE, NULL));
	g_node_append(root, else_node);

	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	if (token->type == T_BEGIN) {
		*tokens = (*tokens)->next;
		ast_recursive(else_node, tokens);
	} else if (token->type == T_IF) {
		ast_recursive(else_node, tokens);
	} else {
		ast_recursive_stmt(else_node, tokens);
	}
}

void
ast_if(GNode * root, GList ** tokens)
{
	Token          *token;
	GNode          *if_node;

	if_node = g_node_new(ast_node_new(T_IF, NULL));
	g_node_append(root, if_node);

	ast_expression(if_node, tokens, T_THEN);

	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	if (token->type == T_BEGIN) {
		*tokens = (*tokens)->next;
		ast_recursive(if_node, tokens);
	} else {
		ast_recursive_stmt(if_node, tokens);
	}
	
	token = (Token *)(*tokens)->data;
	if (token->type == T_ELSE) {
		ast_else(if_node, tokens);
	}
}

void
ast_while(GNode * root, GList ** tokens)
{
	GNode          *while_node;

	while_node = g_node_new(ast_node_new(T_WHILE, NULL));
	g_node_append(root, while_node);
	
	ast_expression(while_node, tokens, T_DO);
	ast_recursive_stmt(while_node, tokens);
}

void
ast_read(GNode * root, GList ** tokens)
{
	Token          *token;

	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	switch (symbol_table_get_entry_kind(symbol_table, token->id)) {
		case SK_INTEGER:
			break;
		case SK_NONE:
			ast_error_token(token, "undefined symbol");
			break;
		default:
			ast_error_token(token, "variable is not integer");
	}

	g_node_append(root, g_node_new(ast_node_new(T_READ, token->id)));

	*tokens = (*tokens)->next;	/* skip T_IDENTIFIER */
}

void
ast_write(GNode * root, GList ** tokens)
{
	Token          *token;

	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;
	
	switch (symbol_table_get_entry_kind(symbol_table, token->id)) {
		case SK_INTEGER:
			break;
		case SK_NONE:
			ast_error_token(token, "undefined symbol");
			break;
		default:
			ast_error_token(token, "variable is not integer");
	}
	
	g_node_append(root, g_node_new(ast_node_new(T_WRITE, token->id)));

	*tokens = (*tokens)->next;	/* skip T_IDENTIFIER */
}

void
ast_identifier(GNode *root, GList **tokens)
{
	Token *token, *prev_token;
	gchar *symbol;
	
	prev_token = token = (Token *)(*tokens)->data;
	symbol = token->id;
	
	switch (symbol_table_get_entry_type(symbol_table, token->id)) {
		case ST_PROCEDURE:
			*tokens = (*tokens)->next;
			token = (Token *)(*tokens)->data;
			
			if (token->type != T_SEMICOLON)
				ast_error_token(prev_token, "cannot assign to a procedure");
		
			g_node_append(root, g_node_new(ast_node_new(T_PROCEDURE_CALL, symbol)));
			break;
		case ST_FUNCTION:
			*tokens = (*tokens)->next;
			token = (Token *)(*tokens)->data;
			
			if (token->type != T_ATTRIB)
				ast_error_token(prev_token, "functions can only be called in an assignment");
				
			break;
		case ST_VARIABLE:
			*tokens = (*tokens)->next;
			break;
		default:
			ast_error_token(token, "undefined symbol");
	}
}

void
ast_begin(GNode * root, GList ** tokens)
{
	*tokens = (*tokens)->next;
	
	ast_recursive(root, tokens);
}

void            inline
ast_recursive_stmt(GNode * root, GList ** token_list)
{
	__ast_recursive(root, token_list, T_SEMICOLON);
}

void            inline
ast_recursive(GNode * root, GList ** token_list)
{
	__ast_recursive(root, token_list, T_END);
}

void
__ast_recursive(GNode * root, GList ** token_list, TokenType end_with)
{
	Token          *token;

	for (; *token_list;) {
		token = (Token *) (*token_list)->data;

		switch (token->type) {
		case T_VAR:
			ast_var(root, token_list);
			break;
		case T_IDENTIFIER:
			ast_identifier(root, token_list);
			break;
		case T_BEGIN:
			ast_begin(root, token_list);
			break;
		case T_ATTRIB:
			ast_attrib(root, token_list);
			break;
		case T_IF:
			ast_if(root, token_list);
			break;
		case T_WHILE:
			ast_while(root, token_list);
			break;
		case T_READ:
			ast_read(root, token_list);
			break;
		case T_WRITE:
			ast_write(root, token_list);
			break;
		case T_FUNCTION:
			ast_function(root, token_list);
			break;
		case T_PROCEDURE:
			ast_procedure(root, token_list);
			break;
		default:
			*token_list = (*token_list)->next;

			if (token->type == end_with) {
				return;
			}
		}
	}
}

GNode          *
ast(TokenList * token_list)
{
	Token          *token;
	GNode          *ast = NULL;
	GList          *tokens;

	if (!token_list)
		return NULL;

	symbol_table = symbol_table_new();
	
	token = (Token *) (token_list->tokens)->next->data;
	ast = g_node_new(ast_node_new(T_PROGRAM, token->id));

	tokens = token_list->tokens->next->next;
	ast_recursive(ast, &tokens);

	symbol_table_free(symbol_table);

	return ast;
}


#ifdef TEST_AST
gboolean
traverse_func(GNode * node, gpointer data)
{
	if (node->parent) {
		ASTNode        *ast_node = (ASTNode *) node->data, *ast_parent = (ASTNode *) node->parent->data;

		printf("\t\"%s %s (%p)\" -> \"%s %s (%p)\";\n",
		       literals[ast_parent->token], (char *) ast_parent->data, node->parent,
		  literals[ast_node->token], (char *) ast_node->data, node);
	}
	return FALSE;
}

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
	
	tl_unref(token_list);
	/* FIXME liberar token_list? com alguns programas o programa morre quando tenta imprimir
		ate mesmo o endereco da variavel. parece que a ast() ta corrompendo memoria em algum
		canto. preciso descobrir onde. */
	
	puts("digraph ast {");
	g_node_traverse(root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, traverse_func, NULL);
	puts("}");


	return 0;
}
#endif				/* TEST_AST */
