#ifndef __CODEGEN_H__
#define __CODEGEN_H__

#include <glib.h>
#include "lex.h"

typedef struct _Instruction		Instruction;

struct _Instruction {
	TokenType	op;
	gchar		*arg1;
	gchar		*arg2;
	gchar		*result;
};

#endif	 /* __CODEGEN_H__ */