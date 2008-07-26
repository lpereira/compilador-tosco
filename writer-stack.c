/*
 * Simple Pascal Compiler
 * Stack Machine Writer
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#include "writer-stack.h"

static void
stack_prolog(FILE *output)
{
  ;
}

static void
stack_epilog(FILE *output)
{
  ;
}

static void
stack_label(InstructionLabel *i, FILE *output)
{
  fprintf(output, "label%d:\n", i->number);
}

static void
stack_goto(InstructionGoto *i, FILE *output)
{
  fprintf(output, "    goto label%d\n", i->label);
}

static void
stack_ifnot(InstructionIfNot *i, FILE *output)
{
  fprintf(output, "    if_not goto label%d\n", i->label);
}

static void
stack_if(InstructionIf *i, FILE *output)
{
  fprintf(output, "    if goto label%d\n", i->label);
}

static void
stack_ldimed(InstructionLdImed *i, FILE *output)
{
  fprintf(output, "    pushc %d\n", i->value);
}

static void
stack_ldreg(InstructionLdReg *i, FILE *output)
{
  fprintf(output, "    t[%d] = t[%d];\n", i->reg1, i->reg2);
}

static void
stack_alloc(InstructionAlloc *i, FILE *output)
{
  fprintf(output, "    alloc %d\n", i->bytes);
}

static void
stack_free(InstructionFree *i, FILE *output)
{
  fprintf(output, "    free %d\n", i->bytes);
}

static void
stack_binop(InstructionBinOp *i, FILE *output)
{
  char *op;
  
  if (g_str_equal(i->op, "=")) {
    op = "==";
  } else if (g_str_equal(i->op, "+")) {
    op = "add";
  } else if (g_str_equal(i->op, "-")) {
    op = "sub";
  } else if (g_str_equal(i->op, "/")) {
    op = "div";
  } else if (g_str_equal(i->op, "*")) {
    op = "mul";
  } else {
    op = i->op;
  }
  
  fprintf(output, "    %s\n", op);
}

static void
stack_unop(InstructionUnOp *i, FILE *output)
{
  fprintf(output, "    push t%d\n", i->reg);
  fprintf(output, "    %s\n", i->op);
}

static void
stack_load(InstructionLoad *i, FILE *output)
{
  fprintf(output, "    load (mp-%d, %d)\n", i->offset, i->size);
}

static void
stack_store(InstructionStore *i, FILE *output)
{
  fprintf(output, "    store (mp-%d, %d)\n", i->offset, i->size);
}

static void
stack_read(InstructionRead *i, FILE *output)
{
  fprintf(output, "    read\n");
}

static void
stack_write(InstructionWrite *i, FILE *output)
{
  fprintf(output, "    print\n");
}

static void
stack_return(InstructionReturn *i, FILE *output)
{
  fprintf(output, "    return\n");
}

static void
stack_returnv(InstructionReturnV *i, FILE *output)
{
  ;
}

static void
stack_copyrv(InstructionCopyRetV *i, FILE *output)
{
  fprintf(output, "    dup\n");
}

static void
stack_pushreg(InstructionPushReg *i, FILE *output)
{
  fprintf(output, "    stack[++sp] = t[%d];\n", i->reg);
}

static void
stack_popreg(InstructionPopReg *i, FILE *output)
{
  fprintf(output, "    t[%d] = stack[sp--];\n", i->reg);
}

static void
stack_pcall(InstructionPCall *i, FILE *output)
{
  fprintf(output, "    call label%d\n", i->label_number);
}

static void
stack_fcall(InstructionFCall *i, FILE *output)
{
  fprintf(output, "    call label%d\n", i->label_number);
}

EmitterWriter *
writer_stack_get_emitter(void)
{
  static EmitterWriter w = {
    .write_label	= stack_label,
    .write_goto		= stack_goto,
    .write_ifnot	= stack_ifnot,
    .write_if		= stack_if,
    .write_ldimed	= stack_ldimed,
    .write_ldreg	= stack_ldreg,
    .write_alloc	= stack_alloc,
    .write_free		= stack_free,
    .write_binop	= stack_binop,
    .write_unop		= stack_unop,
    .write_load		= stack_load,
    .write_store	= stack_store,
    .write_read		= stack_read,
    .write_write	= stack_write,
    .write_return	= stack_return,
    .write_returnv	= stack_returnv,
    .write_copyrv	= stack_copyrv,
    .write_pushreg	= stack_pushreg,
    .write_popreg	= stack_popreg,
    .write_pcall	= stack_pcall,
    .write_fcall	= stack_fcall,
    .write_epilog	= stack_epilog,
    .write_prolog	= stack_prolog
  };
  
  return &w;
}
