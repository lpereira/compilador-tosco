#include <stdio.h>
#include "charbuf.h"

static GSList	*char_buf = NULL;

void
char_buf_put_char(char ch)
{
	char_buf = g_slist_prepend(char_buf, GINT_TO_POINTER((gint)ch));
}

void
char_buf_put_string(gchar *string)
{
	GSList *temp = NULL;
	
	while (*string) {
		temp = g_slist_append(temp, GINT_TO_POINTER((gint)*string++));
	}

	char_buf = g_slist_concat(temp, char_buf);	
}

int
char_buf_get(void)
{
	if (char_buf == NULL) {
		return getchar();
	} else {
		int ch;
		GSList *next;

		ch = GPOINTER_TO_INT(char_buf->data);

		next = char_buf->next;
		g_slist_free_1(char_buf);

		char_buf = next;

		return ch;	
	}	
}

