#ifndef __AST_H__
#define __AST_H__

#include "lex.h"

typedef struct	_ASTNode	ASTNode;

struct _ASTNode {
  TokenType token;
  gpointer data;
};

GNode          *ast(TokenList * token_list);

#endif	/* __AST_H__ */

