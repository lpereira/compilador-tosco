/*
 * Simple Pascal Compiler
 * Code Generator
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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

static guint __label_value = 0;
static Stack *context = NULL, *n_vars = NULL, *ret_var = NULL;
static gboolean has_procedure_or_function = FALSE;
/***/

static void generate(GNode * node);

/***/

static void emit(char *label, char *instruction, char *p1, char *p2)
{
    printf("%-3s %-7s %-3s %-3s\n",
	   label ? label : "", instruction, p1 ? p1 : "", p2 ? p2 : "");
}

static guint label_new(void)
{
    return ++__label_value;
}

static guint generate_children(GNode * nodes)
{
    GNode *n;

    for (n = nodes->children; n; n = n->next) {
	generate(n);
    }

    return 0;
}

static int available_address = 0;

static int generate_var(GNode * types)
{
    GNode *type, *var;
    gint size = 1, n_vars;
    guint n_elements = 0;
    gchar arg1[8], arg2[8];

    sprintf(arg1, "%d", available_address);

    for (type = types->children; type; type = type->next) {
	ASTNode *ast_node = (ASTNode *) type->data;

	for (n_vars = 0, var = type->children; var; var = var->next, n_vars++) {
	    ast_node = (ASTNode *) var->data;

	    symbol_table_set_attribute_int(symbol_table,
					   (gchar *) ast_node->data,
					   STF_MEMORY_ADDRESS,
					   available_address);
	    available_address += size;
	}

	n_elements += n_vars;
    }

    sprintf(arg2, "%d", n_elements);
    emit(NULL, "ALLOC", arg1, arg2);

    return n_elements;
}

static void generate_if(GNode * nodes)
{
    GNode *n;
    gchar arg1[8];
    guint l1, l2;
    gboolean has_else = FALSE;

    l1 = label_new();
    l2 = label_new();

    generate(nodes->children);
    sprintf(arg1, "L%X", l1);
    emit(NULL, "JMPF", arg1, NULL);

    for (n = nodes->children->next; n; n = n->next) {
	ASTNode *ast_node = (ASTNode *) n->data;

	if (ast_node->token != T_ELSE) {
	    generate(n);
	} else {
	    sprintf(arg1, "L%X", l2);
	    emit(NULL, "JMP", arg1, NULL);

	    sprintf(arg1, "L%X", l1);
	    emit(arg1, "NULL", NULL, NULL);
	    generate_children(n);

	    has_else = TRUE;
	}
    }

    if (has_else) {
	sprintf(arg1, "L%X", l2);
	emit(arg1, "NULL", NULL, NULL);
    } else {
	sprintf(arg1, "L%X", l1);
	emit(arg1, "NULL", NULL, NULL);
    }
}

static void generate_while(GNode * nodes)
{
    GNode *n;
    gchar arg1[8];
    guint l1, l2;

    l1 = label_new();
    l2 = label_new();

    sprintf(arg1, "L%X", l1);
    emit(arg1, "NULL", NULL, NULL);
    generate(nodes->children);

    sprintf(arg1, "L%X", l2);
    emit(NULL, "JMPF", arg1, NULL);

    for (n = nodes->children->next; n; n = n->next) {
	generate(n);
    }

    sprintf(arg1, "L%X", l1);
    emit(NULL, "JMP", arg1, NULL);

    sprintf(arg1, "L%X", l2);
    emit(arg1, "NULL", NULL, NULL);
}

static void generate_binop(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    char *op = "";

    if (g_str_equal(literals[ast_node->token], "=")) {
	op = "CEQ";
    } else if (g_str_equal(literals[ast_node->token], "<>")) {
	op = "CDIF";
    } else if (g_str_equal(literals[ast_node->token], ">")) {
	op = "CMA";
    } else if (g_str_equal(literals[ast_node->token], "<")) {
	op = "CME";
    } else if (g_str_equal(literals[ast_node->token], "<=")) {
	op = "CMEQ";
    } else if (g_str_equal(literals[ast_node->token], ">=")) {
	op = "CMAQ";
    } else if (g_str_equal(literals[ast_node->token], "+")) {
	op = "ADD";
    } else if (g_str_equal(literals[ast_node->token], "-")) {
	op = "SUB";
    } else if (g_str_equal(literals[ast_node->token], "*")) {
	op = "MULT";
    } else if (g_str_equal(literals[ast_node->token], "div")) {
	op = "DIVI";
    } else if (g_str_equal(literals[ast_node->token], "ou")) {
	op = "OR";
    } else if (g_str_equal(literals[ast_node->token], "e")) {
	op = "AND";
    }

    generate(node->children);
    generate(node->children->next);

    emit(NULL, op, NULL, NULL);
}

static void generate_number(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;

    emit(NULL, "LDC", (gchar *) ast_node->data, NULL);
}

static void generate_attrib(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    GNode *children = node->children;
    gchar arg1[8];
    guint memory;

    generate(children);

    memory =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "%d", memory);
    emit(NULL, "STR", arg1, NULL);
}

static void generate_attrib_from_temp(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    char arg1[8];
    guint memory;

    memory =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "%d", memory);
    emit(NULL, "STR", arg1, NULL);
}

static void generate_identifier(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    gchar arg1[8];
    guint memory;

    memory =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "%d", memory);
    emit(NULL, "LDV", arg1, NULL);
}

static void generate_read(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    gchar arg1[8];
    guint memory;

    emit(NULL, "RD", NULL, NULL);
    memory =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "%d", memory);
    emit(NULL, "STR", arg1, NULL);
}

static void generate_write(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    gchar arg1[8];
    guint memory;

    memory =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "%d", memory);
    emit(NULL, "LDV", arg1, NULL);
    emit(NULL, "PRN", NULL, NULL);
}

static void generate_function_return(GNode * node)
{
    char arg1[8];

    sprintf(arg1, "%d", GPOINTER_TO_INT(stack_peek(ret_var)));

    generate(node->children);
    
    /* saves the return value */
    emit(NULL, "STR", arg1, NULL);
}

static guint generate_procedure_or_function(GNode * node, guint procedure)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    guint n_var = 0, r, l = 0, context_level;
    char arg1[8], arg2[8];
    GNode *n;
    
    context_level = symbol_table_get_context_level(symbol_table);
    r = label_new();

    if (!has_procedure_or_function) {
        has_procedure_or_function = TRUE;
        
        emit(NULL, "JMP", "PRG", NULL);
    }
    
    if (context_level > 1) {
        l = label_new();
        sprintf(arg2, "L%X", l);
        emit(NULL, "JMP", arg2, NULL);
    }

    sprintf(arg2, "L%X", r);
    emit(arg2, "NULL", NULL, NULL);

    symbol_table_set_attribute_int(symbol_table, (gchar *) ast_node->data,
				   STF_MEMORY_ADDRESS, r);
    symbol_table_context_enter(symbol_table, (gchar *) ast_node->data);

    if (!procedure) {
	stack_push(n_vars, GUINT_TO_POINTER(n_var));

	/* allocate memory for return address */
	stack_push(ret_var, GUINT_TO_POINTER(available_address));

	sprintf(arg1, "%d", available_address);
	sprintf(arg2, "1");
	
	emit(NULL, "ALLOC", arg1, arg2);

	available_address++;
    }

    for (n = node->children; n; n = n->next) {
	ast_node = (ASTNode *) n->data;

	if (ast_node->token == T_VAR) {
	    n_var = generate_var(n);

	    if (!procedure && n_var) {
		/* remove the "zero vars" */
		stack_pop(n_vars);
		/* add the number of variables */
		stack_push(n_vars, GUINT_TO_POINTER(n_var));
	    }
	} else {
	    generate(n);
	}
    }

    if (procedure) {
        if (n_var) {
            sprintf(arg1, "%d", available_address - n_var);
            sprintf(arg2, "%d", n_var);

            emit(NULL, "DALLOC", arg1, arg2);
            available_address -= n_var;
        }

	emit(NULL, "RETURN", NULL, NULL);
    } else {
        if ((n_var = GPOINTER_TO_INT(stack_peek(n_vars)))) {
            gchar arg1[8], arg2[8];

            sprintf(arg1, "%d", available_address - n_var);
            sprintf(arg2, "%d", n_var);

            emit(NULL, "DALLOC", arg1, arg2);
        }

        emit(NULL, "RETURNF", arg1, NULL);

	available_address -= GPOINTER_TO_INT(stack_pop(n_vars));
	available_address--;

	stack_pop(ret_var);
    }

    symbol_table_context_leave(symbol_table);

    if (context_level > 1) {
        sprintf(arg2, "L%X", l);
        emit(arg2, "NULL", NULL, NULL);
    }

    return 0;
}

static guint generate_procedure(GNode * node)
{
    return generate_procedure_or_function(node, TRUE);
}

static void generate_for(GNode * nodes)
{
    GNode *n, *step, *child, *loop_var;
    guint l1, l2;
    char arg1[8];

    loop_var = child = nodes->children;

    l1 = label_new();
    l2 = label_new();

    /* start value */
    generate_attrib(child);
    child = child->next;

    /* condition */
    sprintf(arg1, "L%X", l1);
    emit(arg1, "NULL", NULL, NULL);
    generate(child);
    sprintf(arg1, "L%X", l2);
    emit(NULL, "JMPF", arg1, NULL);

    /* step */
    step = child = child->next;

    /* commands */
    child = child->next;
    for (n = child; n; n = n->next) {
	generate(n);
    }

    /* step (cont'd) */
    generate(step);
    generate_identifier(loop_var);
    emit(NULL, "ADD", NULL, NULL);
    generate_attrib_from_temp(loop_var);

    sprintf(arg1, "L%X", l1);
    emit(NULL, "JMP", arg1, NULL);
    sprintf(arg1, "L%X", l2);
    emit(arg1, "NULL", NULL, NULL);
}

static void generate_procedure_call(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    char arg1[8];
    guint label;

    label =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "L%X", label);
    emit(NULL, "CALL", arg1, NULL);
}

static guint generate_function(GNode * node)
{
    return generate_procedure_or_function(node, FALSE);
}

static void generate_function_call(GNode * node)
{
    ASTNode *ast_node = (ASTNode *) node->data;
    char arg1[8];
    guint label;

    label =
	symbol_table_get_attribute_int(symbol_table,
				       (gchar *) ast_node->data,
				       STF_MEMORY_ADDRESS);

    sprintf(arg1, "L%X", label);
    emit(NULL, "CALL", arg1, NULL);
}

static void generate_true(GNode * node)
{
    emit(NULL, "LDC", "1", NULL);
}

static void generate_false(GNode * node)
{
    emit(NULL, "LDC", "0", NULL);
}

static void generate_not(GNode * node)
{
    generate(node->children);
    emit(NULL, "NEG", NULL, NULL);
}

static void generate_uminus(GNode * node)
{
    generate(node->children);
    emit(NULL, "INV", NULL, NULL);
}

static void generate_uplus(GNode * node)
{
    generate(node->children);
}

static void generate_main_begin(GNode *node)
{
    if (has_procedure_or_function) {
        emit("PRG", "NULL", NULL, NULL);
    }
}

static void generate(GNode * node)
{
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
    case T_OP_DIFFERENT:
    case T_OR:
    case T_AND:
	generate_binop(node);
	break;
    case T_NOT:
	generate_not(node);
	break;
    case T_UNARY_PLUS:
	generate_uplus(node);
	break;
    case T_UNARY_MINUS:
	generate_uminus(node);
	break;
    case T_TRUE:
	generate_true(node);
	break;
    case T_FALSE:
	generate_false(node);
	break;
    case T_NUMBER:
	generate_number(node);
	break;
    case T_ATTRIB:
	generate_attrib(node);
	break;
    case T_VAR:
	generate_var(node);
	break;
    case T_IDENTIFIER:
	generate_identifier(node);
	break;
    case T_READ:
	generate_read(node);
	break;
    case T_WRITE:
	generate_write(node);
	break;
    case T_WHILE:
	generate_while(node);
	break;
    case T_FOR:
	generate_for(node);
	break;
    case T_IF:
	generate_if(node);
	break;
    case T_PROGRAM:
	generate_children(node);
	break;
    case T_PROCEDURE:
	generate_procedure(node);
	break;
    case T_PROCEDURE_CALL:
	generate_procedure_call(node);
	break;
    case T_FUNCTION:
	generate_function(node);
	break;
    case T_FUNCTION_CALL:
	generate_function_call(node);
	break;
    case T_FUNCTION_RETURN:
	generate_function_return(node);
	break;
    case T_MAIN_BEGIN:
        generate_main_begin(node);
        break;
    default:
	g_error("Houston, we have a problem! Don't know how to "
		"generate code for node type ``%s'' (%d).",
		literals[ast_node->token], ast_node->token);
    }
}

void codegen(GNode * root)
{
    __label_value = 0;

    context = stack_new();
    n_vars = stack_new();
    ret_var = stack_new();

    symbol_table_context_reset(symbol_table);

    emit(NULL, "START", NULL, NULL);
    generate(root);

    if (available_address) {
	char imed[16];

	snprintf(imed, 16, "%d", available_address);
	emit(NULL, "DALLOC", "0", imed);
    }

    emit(NULL, "HLT", NULL, NULL);

    stack_free(context);
    stack_free(n_vars);
    stack_free(ret_var);
}

int codegen_test_main(int argc, char **argv)
{
    GNode *root;
    TokenList *token_list;

    token_list = lex();
    root = ast(token_list);

    codegen(root);

    tl_destroy(token_list);

    return 0;
}
