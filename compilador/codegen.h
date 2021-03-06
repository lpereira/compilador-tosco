/*
 * Simple Pascal Compiler
 * Code Generator
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#include <glib.h>

#include "lex.h"

void	 codegen(GNode *node);
int	 codegen_test_main(int argc, char **argv);

#endif	 /* __CODEGEN_H__ */
