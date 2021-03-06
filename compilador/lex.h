/*
 * Simple Pascal Compiler
 * Lexical and Syntatical Analysis
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __LEX_H__
#define __LEX_H__

#include "charbuf.h"
#include "tokenlist.h"

#define LPD		/* define to enable processing of LPD */

extern const char *literals[];

TokenList *lex(void);
int	   lex_test_main(int argc, char **argv);

#endif	/* __LEX_H__ */
