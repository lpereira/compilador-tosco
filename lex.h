#ifndef __LEX_H__
#define __LEX_H__

#include "charbuf.h"
#include "tokenlist.h"

extern const char *literals[];

TokenList *lex(void);
int	   lex_test_main(int argc, char **argv);

#endif	/* __LEX_H__ */
