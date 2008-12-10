/*
 * Simple Pascal Compiler
 * Token List
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#ifndef __TOKENLIST_H__
#define __TOKENLIST_H__

#include <glib.h>

typedef enum {
  T_NONE, T_COMMA,  T_THEN,  T_ATTRIB,  T_BOOLEAN,  T_CLOSEPAREN, 
  T_COLON,
  T_DIVIDE,  T_DO,  T_ELSE,  T_FALSE,  T_END,  T_FUNCTION,  T_IF,  T_BEGIN,
  T_INTEGER,  T_MINUS,  T_MULTIPLY,  T_NOT,  T_OP_DIFFERENT,  T_OPENPAREN,
  T_OP_EQUAL,  T_OP_GT,  T_OP_GEQ,  T_OP_LT,  T_OP_LEQ,  T_OR,  T_PERIOD,
  T_PLUS,  T_PROCEDURE,  T_PROGRAM,  T_READ,  T_SEMICOLON,  T_AND,  T_TRUE,
  T_VAR,  T_WHILE,  T_WRITE,  T_IDENTIFIER,  T_NUMBER, T_FUNCTION_RETURN,
  T_FUNCTION_CALL, T_PROCEDURE_CALL, T_UNARY_MINUS, T_FOR, T_STEP, T_UNARY_PLUS,
  T_MAIN_BEGIN
} TokenType;

typedef struct	_Token		Token;
typedef struct	_TokenList	TokenList;

struct _Token {
  TokenType	type;
  char		*id;
  int		line, column;
};

struct _TokenList {
  GList		*tokens;
  int		ref_count;
};

TokenList	*tl_new(void);
void		 tl_unref(TokenList *t);
void		 tl_ref(TokenList *t);
void		 tl_add_token(TokenList **token_list, TokenList *t);
void		 tl_destroy(TokenList *tl);
TokenList	*tl_new_char(char ch);
void		 tl_add_semicolon(TokenList *token_list, gint line, gint column);

#endif	/* __TOKENLIST_H__ */
