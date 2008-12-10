/*
 * Simple Pascal Compiler
 * Character Buffer
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __CHARBUF_H__
#define __CHARBUF_H__

#include <glib.h>
#include <stdio.h>

void	char_buf_put_char(gchar ch);
void	char_buf_put_string(gchar *string);
int	char_buf_get(void);
void	char_buf_set_file(FILE *f);

#endif	/* __CHARBUF_H__ */
