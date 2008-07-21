/*
 * Simple Pascal Compiler
 * LLVM Writer
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#ifndef __WRITER_LLVM_H__
#define __WRITER_LLVM_H__

#include "codeemitter.h"

EmitterWriter *writer_llvm_get_emitter(void);

#endif	/* __WRITER_LLVM_H__ */
