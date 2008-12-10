/*
 * Simple Pascal Compiler
 * Token List
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#include "tokenlist.h"

TokenList      *
tl_new(void)
{
	TokenList      *tl;

	tl = g_new0(TokenList, 1);
	tl->tokens = NULL;

	return tl;
}

void
tl_unref(TokenList * t)
{
	if (t && --t->ref_count < 0) {
		tl_destroy(t);
	}
}

void
tl_ref(TokenList * t)
{
	t->ref_count++;
}

void
tl_add_token(TokenList ** token_list, TokenList * t)
{
	GList          *token;

	if (t) {
		for (token = t->tokens; token; token = token->next) {
			(*token_list)->tokens = g_list_append((*token_list)->tokens, token->data);
		}

		tl_unref(t);
	}
}

void
tl_destroy(TokenList * tl)
{
	g_list_free(tl->tokens);	
	g_free(tl);
}

void
tl_add_semicolon(TokenList *token_list, gint line, gint column)
{
	Token		*token;

	token = g_new0(Token, 1);
	token->type = T_SEMICOLON;
	token->id = ";";
	token->line = line;
	token->column = column;
	
	token_list->tokens = g_list_append(token_list->tokens, token);
}

