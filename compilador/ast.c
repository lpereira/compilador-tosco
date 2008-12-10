/*
 * Simple Pascal Compiler
 * Abstract Syntax Tree
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 * ast_expression() is based on Exp_to_Tree (Function to Convert Infix Expression to Expression Tree),
 * written by Michael B. Feldman <mfeldman@gwu.edu>.
 *
 * FIXME
 *  - will allow functions without return (!!)
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

/* static prototypes */
static void __ast_recursive(GNode * root, GList ** token_list, TokenType end_with);
static void inline ast_recursive(GNode * root, GList ** token_list);
static void inline ast_recursive_stmt(GNode * root, GList ** token_list);
static SymbolSubType tc_node_subtype(GNode * node);
static SymbolSubType tc_node_subtype_unary(GNode * op, GNode * sub);
static SymbolSubType tc_node_subtype_binary(GNode * op, GNode * left, GNode * right);

static SymbolSubType tc_result_subtype_binary(GNode * op, SymbolSubType left, SymbolSubType right);
static SymbolSubType tc_result_subtype_unary(GNode * op, SymbolSubType sub_type);

/* static global variables */
SymbolTable *symbol_table;
static Stack *funcproc_names;

/**
 * Aborta o programa com uma mensagem de erro, mostrando o token, a linha e a coluna.
 * Aceita formatação no estilo printf().
 *
 * @param token		A estrutura token mais próxima do erro
 * @param message	A string de formatação estilo printf()
 */
static void ast_error_token(Token * token, gchar * message, ...)
{
    gchar *buffer;
    va_list args;

    va_start(args, message);
    buffer = g_strdup_vprintf(message, args);
    va_end(args);

    printf
	("Erro: ln <b>%d</b>, col <b>%d</b>, próximo a <b>%s </b>\342\206\222 %s\n",
	 token->line, token->column, token->id, buffer);

    g_free(buffer);

    exit(1);
}

/**
 * Cria um novo nó para a AST.
 *
 * @param token	Tipo do nó
 * @param data  Dado armazenado no nó
 */
ASTNode *ast_node_new(TokenType token, gpointer data)
{
    ASTNode *node;

    node = g_new0(ASTNode, 1);
    node->token = token;
    node->data = data;

    return node;
}

/*
 * Processa uma lista de tokens de declaração de variáveis. Insere os símbolos na tabela de
 * símbolos e verifica a duplicidade.
 *
 * @param root		Raiz (raiz da AST se global, nó de função ou procedimento caso contrário)
 * @param tokens	Entrada dos tokens; é avançado até o token após a declaração de variáveis
 */
static void ast_var(GNode * root, GList ** tokens)
{
    GNode *var_root, *var_type_root;
    GList *vars = NULL, *v;
    Token *token;
    SymbolSubType subtype;

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
	    subtype = (token->type == T_INTEGER) ? SST_INTEGER : SST_BOOLEAN;
	    var_type_root = g_node_new(ast_node_new(token->type, NULL));

	    g_node_append(var_root, var_type_root);

	    for (v = vars; v; v = v->next) {
		token = (Token *) v->data;

		if (symbol_table_is_defined(symbol_table, token->id, params.viagem_do_freitas ? 2 : 1)) {
		    ast_error_token(token, "símbolo duplicado");
		}

		ASTNode *ast_node = ast_node_new(token->type, token->id);

		g_node_append(var_type_root, g_node_new(ast_node));
		symbol_table_install(symbol_table, token->id, ST_VARIABLE, subtype);
	    }

	    g_list_free(vars);
	    vars = NULL;
	    break;
	default:
	    /*
	     * nao vamos fazer nada com esse token, devolve-o
	     * para a lista
	     */
	    //  *tokens = g_list_previous(*tokens);
	    return;
	}
    }
}

/**
 * Encontra o final de uma declaração de função ou procedimento
 *
 * @param tokens Lista de tokens apontando para a declaração atual da função ou procedimento
 * @returns Ponteiro para o final da função ou procedimento
 */
static GList *ast_find_funproc_end(GList * tokens)
{
    gint begin_count = 0;
    Token *token;

    for (tokens = tokens->next;; tokens = tokens->next) {
	token = (Token *) tokens->data;

	if (token->type == T_FUNCTION || token->type == T_PROCEDURE) {
	    tokens = ast_find_funproc_end(tokens->next);
	} else if (token->type == T_BEGIN) {
	    begin_count++;
	} else if (token->type == T_END) {
	    begin_count--;

	    if (begin_count <= 0)
		break;
	}
    }

    return tokens;
}

/**
 * Cria um nó de função.
 * @param root		Raiz (raiz da AST se global, nó de função ou procedimento caso contrário)
 * @param tokens	Entrada dos tokens; é avançado até o token após a declaração da função
 */
static void ast_function(GNode * root, GList ** tokens)
{
    GNode *func_node;
    Token *token;
    GList *t;
    char *function_name;

    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;

    function_name = token->id;

    if (symbol_table_is_defined(symbol_table, function_name, 1)
	|| (stack_peek(funcproc_names)
	    && g_str_equal(stack_peek(funcproc_names), function_name))) {
	ast_error_token(token, "símbolo duplicado");
    }

    func_node = g_node_new(ast_node_new(T_FUNCTION, token->id));
    stack_push(funcproc_names, function_name);
    g_node_append(root, func_node);

    symbol_table_install(symbol_table, function_name, ST_FUNCTION,
			 ((Token *) (*tokens)->next->data)->type == T_INTEGER ? SST_INTEGER : SST_BOOLEAN);
    symbol_table_context_enter(symbol_table, function_name);

    *tokens = (*tokens)->next;
    *tokens = (*tokens)->next;
    switch (((Token *) (*tokens)->data)->type) {
    case T_BEGIN:
	*tokens = (*tokens)->next;
	break;
    case T_PROCEDURE:
    case T_FUNCTION:
    case T_VAR:
	for (t = *tokens; t; t = t->next) {
	    token = (Token *) t->data;

	    if (token->type == T_PROCEDURE || token->type == T_FUNCTION) {
		t = ast_find_funproc_end(t);
		token = (Token *) t->data;
	    }

	    if (token->type == T_BEGIN) {
		token->type = T_NONE;
		break;
	    }
	}
    default:
	break;
    }

    ast_recursive(func_node, tokens);

    stack_pop(funcproc_names);
    symbol_table_context_leave(symbol_table);
}

/**
 * Cria um nó de procedimento.
 * @param root		Raiz (raiz da AST se global, nó de função ou procedimento caso contrário)
 * @param tokens	Entrada dos tokens; é avançado até o token após a declaração do procedimento
 */
static void ast_procedure(GNode * root, GList ** tokens)
{
    GNode *proc_node;
    Token *token;
    GList *t;
    char *proc_name;

    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;

    proc_name = token->id;

    if (symbol_table_is_defined(symbol_table, proc_name, 1)
	|| (stack_peek(funcproc_names)
	    && g_str_equal(stack_peek(funcproc_names), proc_name))) {
	ast_error_token(token, "símbolo duplicado");
    }

    proc_node = g_node_new(ast_node_new(T_PROCEDURE, token->id));
    stack_push(funcproc_names, token->id);
    g_node_append(root, proc_node);

    symbol_table_install(symbol_table, proc_name, ST_PROCEDURE, SST_NONE);
    symbol_table_context_enter(symbol_table, proc_name);

    *tokens = (*tokens)->next;

    switch (((Token *) (*tokens)->data)->type) {
    default:
	break;
    case T_BEGIN:
	*tokens = (*tokens)->next;
	break;
    case T_VAR:
    case T_FUNCTION:
    case T_PROCEDURE:
	for (t = *tokens; t; t = t->next) {
	    token = (Token *) t->data;

	    if (token->type == T_PROCEDURE || token->type == T_FUNCTION) {
		t = ast_find_funproc_end(t);
		token = (Token *) t->data;
	    }

	    if (token->type == T_BEGIN) {
		token->type = T_NONE;
		break;
	    }
	}
    }

    ast_recursive(proc_node, tokens);
    stack_pop(funcproc_names);

    symbol_table_context_leave(symbol_table);
}

/**
 * Retorna o tipo do token associdado com um nó.
 * @param node		A estrutura GNode associada com um nó da AST
 * @returns		O tipo do token associado com o nó //node//.
 */
static TokenType inline ast_node_token(GNode * node)
{
    ASTNode *ast_node;

    return (node && ((ast_node = (ASTNode *) node->data))) ? ast_node->token : T_NONE;
}

/**
 * Verifica o tipo de uma expressão binária.
 * @param op 	Nó do operador
 * @param left	Subtipo do operando do lado esquerdo do operador
 * @param right	Subtipo do operando do lado direito do operador
 * @returns	Subtipo da operação ou SST_NONE caso exista discrepância de tipos
 */
static SymbolSubType tc_result_subtype_binary(GNode * op, SymbolSubType left, SymbolSubType right)
{
    if (left == SST_INTEGER && right == SST_INTEGER) {
	switch (ast_node_token(op)) {
	case T_OP_DIFFERENT:
	case T_OP_EQUAL:
	case T_OP_GT:
	case T_OP_GEQ:
	case T_OP_LT:
	case T_OP_LEQ:
	    return SST_BOOLEAN;

	case T_DIVIDE:
	case T_MINUS:
	case T_MULTIPLY:
	case T_PLUS:
	    return SST_INTEGER;

	default:
	    /* expecting either relop or arith op */
	    return SST_NONE;
	}
    }

    if (left == SST_BOOLEAN && right == SST_BOOLEAN) {
	switch (ast_node_token(op)) {
	case T_OP_DIFFERENT:
	case T_OP_EQUAL:
	case T_OR:
	case T_AND:
	    return SST_BOOLEAN;

	default:
	    /* expecting relop */
	    return SST_NONE;
	}
    }

    /* type mismatch */
    return SST_NONE;
}

/**
 * Verifica o tipo de uma expressão unária.
 * @param op 		Nó do operador
 * @param sub_type	Subtipo do operando do lado esquerdo do operador
 * @returns		Subtipo da operação ou SST_NONE caso exista discrepância de tipos
 */
static SymbolSubType tc_result_subtype_unary(GNode * op, SymbolSubType sub_type)
{
    ASTNode *op_node = (ASTNode *) op->data;

    if (op_node->token == T_UNARY_MINUS && sub_type == SST_INTEGER)
	return SST_INTEGER;

    if (op_node->token == T_UNARY_PLUS && sub_type == SST_INTEGER)
	return SST_INTEGER;

    if (op_node->token == T_NOT && sub_type == SST_BOOLEAN)
	return SST_BOOLEAN;

    /* type mismatch */
    return SST_NONE;
}

static SymbolSubType tc_node_subtype_unary(GNode * op, GNode * sub)
{
    SymbolSubType sub_type;

    sub_type = tc_node_subtype(sub);

    return tc_result_subtype_unary(op, sub_type);
}

static SymbolSubType tc_node_subtype_binary(GNode * op, GNode * left, GNode * right)
{
    SymbolSubType left_subtype, right_subtype;

    left_subtype = tc_node_subtype(left);
    right_subtype = tc_node_subtype(right);

    return tc_result_subtype_binary(op, left_subtype, right_subtype);
}

static SymbolSubType tc_node_subtype(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;

    if (G_NODE_IS_LEAF(node)) {
	switch (ast_node->token) {
	case T_NUMBER:
	    return SST_INTEGER;
	case T_TRUE:
	case T_FALSE:
	    return SST_BOOLEAN;
	case T_IDENTIFIER:
	case T_FUNCTION_CALL:
	    return symbol_table_get_attribute_int(symbol_table, (char *) ast_node->data, STF_SUBTYPE);
	default:
	    /* WTF? */
	    return SST_NONE;
	}
    }

    if (g_node_n_children(node) == 1) {
	return tc_node_subtype_unary(node, node->children);
    } else {
	return tc_node_subtype_binary(node, node->children, node->children->next);
    }
}


static int inline __op_priority(TokenType op)
{
/*
	FIXME

	+u -u nao (1)			5
	div * (2)			4
	+ - (3) 			3
	> >= < <= <> = (4)		2
	<>, = (4)			2
	e, ou (6)			1
*/

    switch (op) {
    case T_UNARY_PLUS:
    case T_UNARY_MINUS:
    case T_NOT:
      return 5;
    case T_DIVIDE:
    case T_MULTIPLY:
      return 4;
    case T_PLUS:
    case T_MINUS:
      return 3;
    case T_OP_GT:
    case T_OP_GEQ:
    case T_OP_LT:
    case T_OP_LEQ:
    case T_OP_DIFFERENT:
    case T_OP_EQUAL:
      return 2;
    case T_AND:
    case T_OR:
      return 1;
    default:
      return 0;
    }

/*    switch (op) {
    default:
	break;
    case T_OP_DIFFERENT:
    case T_OP_EQUAL:
    case T_OP_GT:
    case T_OP_LT:
    case T_OP_GEQ:
    case T_OP_LEQ:
    case T_OR:
	return 0;
    case T_PLUS:
    case T_MINUS:
	return 1;
    case T_MULTIPLY:
    case T_DIVIDE:
    case T_AND:
	return 2;
    case T_NOT:
    case T_UNARY_PLUS:
    case T_UNARY_MINUS:
	return 3;
    }
*/
    return 0;
}

static void pop_connect_push(Stack * op_stack, Stack * node_stack)
{
    TokenType token_type;
    GNode *temp, *left, *right;

    temp = (GNode *) stack_pop(op_stack);

    token_type = ast_node_token(temp);
    if (token_type == T_UNARY_MINUS || token_type == T_UNARY_PLUS || token_type == T_NOT) {
	if ((left = (GNode *) stack_pop(node_stack))) {
	    g_node_append(temp, left);
	} else {
	    /* isso não é pra acontecer, mas não custa nada colocar uma guarda */
	    ast_error_token(temp->data, "expressão unária inválida");
	}
    } else {
	right = (GNode *) stack_pop(node_stack);
	left = (GNode *) stack_pop(node_stack);

	if (left) {
	    g_node_append(temp, left);
	}

	if (right) {
	    g_node_append(temp, right);
	}
    }

    stack_push(node_stack, temp);
}

static void ast_expression(GNode * root, GList ** tokens, TokenType stop)
{
    Token *token;
    Stack *op_stack, *node_stack;

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
	case T_FALSE:
	case T_TRUE:
	    stack_push(node_stack, g_node_new(ast_node_new(token->type, token->id)));
	    break;
	case T_IDENTIFIER:
	    switch (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE)) {
	    case ST_VARIABLE:
		break;
	    case ST_FUNCTION:
		token->type = T_FUNCTION_CALL;
		break;
	    case ST_NONE:
		ast_error_token(token, "símbolo indefinido");
		break;
	    default:
		ast_error_token(token, "não é variável ou função");
	    }

	    stack_push(node_stack, g_node_new(ast_node_new(token->type, token->id)));
	    break;
	case T_NOT:
	case T_UNARY_MINUS:
	case T_UNARY_PLUS:
	case T_PLUS:
	case T_MINUS:
	case T_MULTIPLY:
	case T_DIVIDE:
	case T_OP_DIFFERENT:
	case T_OP_EQUAL:
	case T_OP_GT:
	case T_OP_LT:
	case T_OP_GEQ:
	case T_OP_LEQ:
	case T_OR:
	case T_AND:
	    if (stack_is_empty(op_stack)) {
		stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
	    } else if (ast_node_token(stack_peek(op_stack)) == T_OPENPAREN) {
		stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
	    } else if (__op_priority(ast_node_token(stack_peek(op_stack)))
		       < __op_priority(token->type)) {
		stack_push(op_stack, g_node_new(ast_node_new(token->type, token->id)));
	    } else {
		do {
		    pop_connect_push(op_stack, node_stack);
		} while (!(stack_is_empty(op_stack)
			   || ast_node_token(stack_peek(op_stack)) ==
			   T_OPENPAREN
			   || __op_priority(ast_node_token(stack_peek(op_stack))) < __op_priority(token->type)));

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

static void ast_attrib(GNode * root, GList ** tokens)
{
    Token *token;
    GNode *attrib_node;
    ASTNode *ast_node = NULL;
    SymbolSubType token_subtype;

    token = (Token *) (*tokens)->prev->data;

    if (symbol_table_is_installed(symbol_table, token->id)) {
	switch (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE)) {
	case ST_VARIABLE:
	    ast_node = ast_node_new(T_ATTRIB, token->id);
	    break;
	case ST_FUNCTION:
	    ast_node = ast_node_new(T_FUNCTION_RETURN, token->id);

	    if (!g_str_equal(stack_peek(funcproc_names), token->id)) {
		ast_error_token(token,
				"retorno de <b>%s</b> não permitido em <b>%s</b>",
				token->id, stack_peek(funcproc_names));
	    }
	    break;
	default:
	    ast_error_token(token, "não é variável ou função");
	}

	attrib_node = g_node_new(ast_node);
	g_node_append(root, attrib_node);

	ast_expression(attrib_node, tokens, T_SEMICOLON);

	token_subtype = symbol_table_get_attribute_int(symbol_table, token->id, STF_SUBTYPE);
	if (token_subtype != tc_node_subtype(attrib_node->children)) {
	    ast_error_token(token, "esperando expressão do tipo <b>%s</b>", symbol_subtypes[token_subtype]);
	}

    } else {
	ast_error_token(token, "símbolo indefinido");
    }
}

static void ast_else(GNode * root, GList ** tokens)
{
    Token *token;
    GNode *else_node;

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

static void ast_if(GNode * root, GList ** tokens)
{
    Token *token = (Token *) (*tokens)->data;
    GNode *if_node;

    if_node = g_node_new(ast_node_new(T_IF, NULL));
    g_node_append(root, if_node);

    ast_expression(if_node, tokens, T_THEN);

    if (tc_node_subtype(if_node->children) != SST_BOOLEAN) {
	ast_error_token(token, "expressão não booleana");
    }

    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;

    if (token->type == T_BEGIN) {
	*tokens = (*tokens)->next;
	ast_recursive(if_node, tokens);
    } else if (token->type != T_ELSE) {
	ast_recursive_stmt(if_node, tokens);
    }

    token = (Token *) (*tokens)->data;
    if (token->type == T_ELSE) {
	ast_else(if_node, tokens);
    }
}

static void ast_while(GNode * root, GList ** tokens)
{
    Token *token = (Token *) (*tokens)->data;
    GNode *while_node;

    while_node = g_node_new(ast_node_new(T_WHILE, NULL));
    g_node_append(root, while_node);

    ast_expression(while_node, tokens, T_DO);
    if (tc_node_subtype(while_node->children) != SST_BOOLEAN) {
	ast_error_token(token, "expressão não booleana");
    }

    ast_recursive_stmt(while_node, tokens);
}

static void ast_for(GNode * root, GList ** tokens)
{
    GNode *for_node, *var_node = NULL;
    Token *token;

    /* for */
    for_node = g_node_new(ast_node_new(T_FOR, NULL));
    g_node_append(root, for_node);

    /* variable */
    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;
    if (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE)
	== ST_VARIABLE) {
	var_node = g_node_new(ast_node_new(T_ATTRIB, token->id));
    } else {
	ast_error_token(token, "símbolo não definido ou não variável");
    }

    if (symbol_table_get_attribute_int(symbol_table, token->id, STF_SUBTYPE) != SST_INTEGER) {
	ast_error_token(token, "variável não é do tipo inteiro");
    }

    g_node_append(for_node, var_node);

    /* start value */
    ast_expression(var_node, tokens, T_WHILE);
    if (tc_node_subtype(var_node->children) != SST_INTEGER) {
	ast_error_token(token, "expressão inicializadora do \"para\" não é do tipo inteiro");
    }

    /* final value */
    ast_expression(for_node, tokens, T_SEMICOLON);
    if (tc_node_subtype(for_node->children->next) != SST_BOOLEAN) {
	ast_error_token(token, "condição de repetição do \"para\" não é do tipo booleano");
    }
    *tokens = (*tokens)->next;

    /* step */
    token = (Token *) (*tokens)->data;
    if (token->type == T_STEP) {
	ast_expression(for_node, tokens, T_SEMICOLON);
	if (tc_node_subtype(for_node->children->next->next) != SST_INTEGER) {
	    ast_error_token(token, "passo do \"para\" não é do tipo inteiro");
	}
	/* consume T_DO */
	*tokens = (*tokens)->next;
    } else {
	g_node_append(for_node, g_node_new(ast_node_new(T_NUMBER, "1")));
    }

    /* statement */
    ast_recursive_stmt(for_node, tokens);
}

static void ast_read(GNode * root, GList ** tokens)
{
    Token *token;

    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;

    switch (symbol_table_get_attribute_int(symbol_table, token->id, STF_SUBTYPE)) {
    case SST_INTEGER:
	if (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE) != ST_VARIABLE) {
	    ast_error_token(token, "esperando %s e nao %s", symbol_types[ST_VARIABLE], symbol_types[ST_FUNCTION]);
	}
	break;
    case SST_NONE:
	if (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE) == ST_PROGRAM) {
	    ast_error_token(token, "símbolo é o nome do programa");
	} else {
	    ast_error_token(token, "símbolo indefinido");
	}
	break;
    default:
	ast_error_token(token, "variável não é do tipo inteiro");
    }

    g_node_append(root, g_node_new(ast_node_new(T_READ, token->id)));

    *tokens = (*tokens)->next;	/* skip T_IDENTIFIER */
}

static void ast_write(GNode * root, GList ** tokens)
{
    Token *token;

    *tokens = (*tokens)->next;
    token = (Token *) (*tokens)->data;

    switch (symbol_table_get_attribute_int(symbol_table, token->id, STF_SUBTYPE)) {
    case SST_INTEGER:
	if (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE) != ST_VARIABLE) {
	    ast_error_token(token, "esperando %s e nao %s", symbol_types[ST_VARIABLE], symbol_types[ST_FUNCTION]);
	}
	break;
    case SST_NONE:
	if (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE) == ST_PROGRAM) {
	    ast_error_token(token, "símbolo é o nome do programa");
	} else {
	    ast_error_token(token, "símbolo indefinido");
	}
	break;
    default:
	ast_error_token(token, "variável não é do tipo inteiro");
    }

    g_node_append(root, g_node_new(ast_node_new(T_WRITE, token->id)));

    *tokens = (*tokens)->next;	/* skip T_IDENTIFIER */
}

static void ast_identifier(GNode * root, GList ** tokens)
{
    Token *token, *prev_token;
    gchar *symbol;

    prev_token = token = (Token *) (*tokens)->data;
    symbol = token->id;

    switch (symbol_table_get_attribute_int(symbol_table, token->id, STF_TYPE)) {
    case ST_PROCEDURE:
	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	if (token->type == T_ATTRIB)
	    ast_error_token(prev_token, "impossível atribuir a um procedimento");

	g_node_append(root, g_node_new(ast_node_new(T_PROCEDURE_CALL, symbol)));
	break;
    case ST_FUNCTION:
	*tokens = (*tokens)->next;
	token = (Token *) (*tokens)->data;

	if (token->type != T_ATTRIB)
	    ast_error_token(prev_token, "funções só podem ser chamadas em atribuições");

	break;
    case ST_VARIABLE:
	*tokens = (*tokens)->next;
	break;
    default:
	ast_error_token(token, "símbolo indefinido");
    }
}

static void ast_begin(GNode * root, GList ** tokens)
{
    *tokens = (*tokens)->next;

    ast_recursive(root, tokens);
}

static void ast_main_begin(GNode * root, GList ** tokens)
{
    GNode *main_begin_node;

    main_begin_node = g_node_new(ast_node_new(T_MAIN_BEGIN, NULL));
    g_node_append(root, main_begin_node);

    *tokens = (*tokens)->next;
}

static void inline ast_recursive_stmt(GNode * root, GList ** token_list)
{
    __ast_recursive(root, token_list, T_SEMICOLON);
}

static void inline ast_recursive(GNode * root, GList ** token_list)
{
    __ast_recursive(root, token_list, T_END);
}

static void __ast_recursive(GNode * root, GList ** token_list, TokenType end_with)
{
    Token *token;

    for (; *token_list;) {
	token = (Token *) (*token_list)->data;

	switch (token->type) {
	case T_VAR:
	    ast_var(root, token_list);
	    break;
	case T_IDENTIFIER:
	    ast_identifier(root, token_list);
	    break;
	case T_MAIN_BEGIN:
	    ast_main_begin(root, token_list);
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
	case T_FOR:
	    ast_for(root, token_list);
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

GNode *ast(TokenList * token_list)
{
    Token *token;
    GNode *ast = NULL;
    GList *tokens;

    if (!token_list)
	return NULL;

    symbol_table = symbol_table_new();
    funcproc_names = stack_new();

    token = (Token *) (token_list->tokens)->next->data;
    ast = g_node_new(ast_node_new(T_PROGRAM, token->id));
    symbol_table_install(symbol_table, token->id, ST_PROGRAM, SST_NONE);
    symbol_table_context_enter(symbol_table, token->id);

    tokens = token_list->tokens->next->next;
    ast_recursive(ast, &tokens);

    stack_free(funcproc_names);

    return ast;
}

static gboolean traverse_func(GNode * node, gpointer data)
{
    static GHashTable *t = NULL;
    static gint count = 0, last_printed = -1;

    if (!t) {
	t = g_hash_table_new(NULL, g_int_equal);
    } else if (!node && !data) {
	g_hash_table_destroy(t);
	return FALSE;
    }

    if (node->parent) {
	ASTNode *ast_node = (ASTNode *) node->data;
	ASTNode *ast_parent = (ASTNode *) node->parent->data;
	gchar *data1, *data2;
	gint lbl1, lbl2;
	gpointer lbl_ptr;

	if ((lbl_ptr = g_hash_table_lookup(t, node->parent))) {
	    lbl1 = GPOINTER_TO_INT(lbl_ptr);
	} else {
	    g_hash_table_insert(t, node->parent, GINT_TO_POINTER(++count));
	    lbl1 = count;
	}

	if ((lbl_ptr = g_hash_table_lookup(t, node))) {
	    lbl2 = GPOINTER_TO_INT(lbl_ptr);
	} else {
	    g_hash_table_insert(t, node, GINT_TO_POINTER(++count));
	    lbl2 = count;
	}

	data1 = (ast_parent->data) ? (gchar *) ast_parent->data : "";
	data2 = (ast_node->data) ? (gchar *) ast_node->data : "";

	data1 = g_str_equal(data1, literals[ast_parent->token]) ? "" : data1;
	data2 = g_str_equal(data2, literals[ast_node->token]) ? "" : data2;

	if (last_printed < lbl1) {
	    printf("\tnode%d [label=\"%s %s\"];\n", lbl1, literals[ast_parent->token], data1);
	    last_printed = lbl1;
	}

	if (last_printed < lbl2) {
	    printf("\tnode%d [label=\"%s %s\"];\n", lbl2, literals[ast_node->token], data2);
	    last_printed = lbl2;
	}

	printf("\tnode%d -> node%d;\n", lbl1, lbl2);
    }
    return FALSE;
}

int ast_test_main(int argc, char **argv)
{
    GNode *root;
    TokenList *token_list;

    token_list = lex();
    root = ast(token_list);

    tl_unref(token_list);
    /* FIXME liberar token_list? com alguns programas o programa morre quando tenta imprimir
       ate mesmo o endereco da variavel. parece que a ast() ta corrompendo memoria em algum
       canto. preciso descobrir onde. */

    puts("digraph ast {");
    g_node_traverse(root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, traverse_func, NULL);
    puts("}");

    traverse_func(NULL, NULL);

    return 0;
}
