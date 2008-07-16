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

static void
fold_constants(GNode *node, GNode *parent)
{
    GNode *lchild, *rchild, *child;
    ASTNode *lan, *ran, *nan, *replace = NULL;

    if (!node)
        return;
    
    lchild = node->children;
    if (!lchild)
        return;
    
    rchild = node->children->next;
    if (!rchild) {
        fold_constants(lchild, node);
        return;
    }
    
    fold_constants(lchild, node);
    fold_constants(rchild, node);
    
    for (child = rchild->children; child; child = child->next) {
        fold_constants(child, node);
    }
    
    lchild = node->children;
    if (!lchild)
        return;
    
    rchild = lchild->next;
    if (!rchild)
        return;
    
    lan = (ASTNode *)lchild->data;
    ran = (ASTNode *)rchild->data;
    nan = (ASTNode *)node->data;
    
    switch (nan->token) {
        case T_PLUS:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 + p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_MINUS:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 - p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_MULTIPLY:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 * p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_DIVIDE:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p2 = atoi((char *)ran->data);
                
                if (p2) {
                    p1 = atoi((char *)lan->data);
                    r = g_strdup_printf("%d", p1 / p2);
                
                    replace = ast_node_new(T_NUMBER, r);
                }
            }
            break;
        case T_OP_DIFFERENT:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 != p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_OP_EQUAL:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 == p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_OP_GT:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 > p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_OP_GEQ:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 >= p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_OP_LT:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 < p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        case T_OP_LEQ:
            if (lan->token == T_NUMBER && ran->token == T_NUMBER) {
                short p1, p2;
                gchar *r;
                
                p1 = atoi((char *)lan->data);
                p2 = atoi((char *)ran->data);
                
                r = g_strdup_printf("%d", p1 <= p2);
                
                replace = ast_node_new(T_NUMBER, r);
            }
            break;
        default:
            ;
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
      /* case T_IF: 
         case T_WHILE: */
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

