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
