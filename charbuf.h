#ifndef __CHARBUF_H__
#define __CHARBUF_H__

#include <glib.h>

void	char_buf_put_char(gchar ch);
void	char_buf_put_string(gchar *string);
int	char_buf_get(void);


#endif	/* __CHARBUF_H__ */
