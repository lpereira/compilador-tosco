/*
 * Simple Pascal Compiler
 * Code Optimizer
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinf.org>
 */

#include <stdlib.h>

#include "optimization-l1.h"

#include "ast.h"
#include "tokenlist.h"

/*
 * TODO
 * - Constant propagation
 * - Strength reduction
 */

static void
fold_constants(GNode *node, GNode *parent)
{
    GNode *lchild, *rchild, *child;
    ASTNode *lan, *ran, *nan, *replace = NULL;

    if (!node || !(lchild = node->children))
        return;
    
    if (!(rchild = node->children->next)) {
        fold_constants(lchild, node);
        return;
    }
    
    fold_constants(lchild, node);
    fold_constants(rchild, node);
    
    for (child = rchild->children; child; child = child->next)
        fold_constants(child, node);
    
    if (!(lchild = node->children) || !(rchild = lchild->next))
        return;

    lan = (ASTNode *)lchild->data;
    ran = (ASTNode *)rchild->data;
    nan = (ASTNode *)node->data;
    
    if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
        short p1, p2;
        gchar *r = NULL;
        
        p1 = atoi((char *)lan->data);
        p2 = atoi((char *)ran->data);
        
        switch (nan->token) {
            case T_PLUS:
                r = g_strdup_printf("%d", p1 + p2);
                break;
            case T_MINUS:
                r = g_strdup_printf("%d", p1 - p2);
                break;
            case T_MULTIPLY:
                r = g_strdup_printf("%d", p1 * p2);
                break;
            case T_DIVIDE:
                /* FIXME: Generate an error when a divide by zero would occur.
                          This is easy, but we don't know the AST node 
                          location, so the error message would be
                          meaningless. Just leave error messages to runtime. */
                if (p2) {
                    r = g_strdup_printf("%d", p1 / p2);
                }
                break;
            case T_OP_DIFFERENT:	
                r = g_strdup_printf("%d", p1 != p2);
                break;
            case T_OP_EQUAL:
                r = g_strdup_printf("%d", p1 == p2);
                break;
            case T_OP_GT:
                r = g_strdup_printf("%d", p1 > p2);
                break;
            case T_OP_GEQ:
                r = g_strdup_printf("%d", p1 >= p2);
                break;
            case T_OP_LT:
                r = g_strdup_printf("%d", p1 < p2);
                break;
            case T_OP_LEQ:
                r = g_strdup_printf("%d", p1 <= p2);
                break;
            default:
                ;
        }
        
        if (r) {
            replace = ast_node_new(T_NUMBER, r);
        }
    }
      
    if (replace) {
        /* FIXME: free up the memory taken by the AST nodes when we destroy their
                  parents. */
        GNode *temp;
        gint position = 0;
        
        for (temp = parent->children; temp; temp = temp->next, position++) {
            if (temp == node) {
                g_node_destroy(node);
                g_node_insert(parent, position, g_node_new(replace));
                return;
            }
        }
    }
}

static gboolean
fold_constants_traverse_func(GNode *node,
                             gpointer data)
{
    ASTNode *ast_node = (ASTNode *)node->data;
    
    switch (ast_node->token) {
      case T_ATTRIB:
      /* case T_IF: 		// if conditions
         case T_WHILE: 		// while conditions
      */
          fold_constants(node, NULL);
          /* fallthrough */
      default:
          ;
    }
    
    return FALSE;
}


void optimization_l1(GNode *ast)
{
    g_node_traverse(ast,
                    G_PRE_ORDER,
                    G_TRAVERSE_NON_LEAVES,
                    -1,
                    fold_constants_traverse_func,
                    NULL);
}

