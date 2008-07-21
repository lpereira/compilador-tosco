/*
 * Simple Pascal Compiler
 * C Writer
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#include "writer-c.h"

static void
c_prolog(FILE *output)
{
  fprintf(output, "#include <stdio.h>\n"
                  "#include <string.h>\n"
                  "#include <setjmp.h>\n"
                  "\n"
                  "int main(int argc, char **argv) {\n"
                  "    unsigned char mem[65536];\n"
                  "    short t[256], tr, csp = -1, stack[256], sp = -1;\n"
                  "    jmp_buf callstack[256];\n"
                  "    unsigned char *mp = mem;\n\n");
}

static void
c_epilog(FILE *output)
{
  fprintf(output, "\n"
                  "    return 0;\n"
                  "}\n");
}

static void
c_label(InstructionLabel *i, FILE *output)
{
  fprintf(output, "label%d:\n", i->number);
}

static void
c_goto(InstructionGoto *i, FILE *output)
{
  fprintf(output, "    goto label%d;\n", i->label);
}

static void
c_ifnot(InstructionIfNot *i, FILE *output)
{
  fprintf(output, "    if (!t[%d])\n", i->reg);
  fprintf(output, "        goto label%d;\n", i->label);
}

static void
c_if(InstructionIf *i, FILE *output)
{
  fprintf(output, "    if (t[%d])\n", i->reg);
  fprintf(output, "        goto label%d;\n", i->label);
}

static void
c_ldimed(InstructionLdImed *i, FILE *output)
{
  fprintf(output, "    t[%d] = %d;\n", i->reg, i->value);
}

static void
c_ldreg(InstructionLdReg *i, FILE *output)
{
  fprintf(output, "    t[%d] = t[%d];\n", i->reg1, i->reg2);
}

static void
c_alloc(InstructionAlloc *i, FILE *output)
{
  fprintf(output, "    mp += %d;\n", i->bytes);
}

static void
c_free(InstructionFree *i, FILE *output)
{
  fprintf(output, "    mp -= %d;\n", i->bytes);
}

static void
c_binop(InstructionBinOp *i, FILE *output)
{
  char *op;
  
  if (g_str_equal(i->op, "=")) {
    op = "==";
  } else {
    op = i->op;
  }
  
  fprintf(output, "    t[%d] = t[%d] %s t[%d];\n", i->result, i->reg1, op, i->reg2);
}

static void
c_unop(InstructionUnOp *i, FILE *output)
{
  char *op;
  
  if (g_str_equal(i->op, "not")) {
    op = "!";
  } else if (g_str_equal(i->op, "neg")) {
    op = "-";
  } else {
    op = i->op;
  }
  
  fprintf(output, "    t[%d] = %s t[%d];\n", i->result, op, i->reg);
}

static void
c_load(InstructionLoad *i, FILE *output)
{
  fprintf(output, "    memcpy(t + %d, mp - %d, %d);\n",
          i->reg, i->offset, i->size);
}

static void
c_store(InstructionStore *i, FILE *output)
{
  fprintf(output, "    memcpy(mp - %d, t + %d, %d);\n",
          i->offset, i->reg, i->size);
}

static void
c_read(InstructionRead *i, FILE *output)
{
  fprintf(output, "    scanf(\"%%d\", &(t[%d]));\n", i->reg);
}

static void
c_write(InstructionWrite *i, FILE *output)
{
  fprintf(output, "    printf(\"%%d\\n\", t[%d]);\n", i->reg);
}

static void
c_return(InstructionReturn *i, FILE *output)
{
  fprintf(output, "    _longjmp(callstack[csp--], 1);\n");
}

static void
c_returnv(InstructionReturnV *i, FILE *output)
{
  fprintf(output, "    tr = t[%d];\n", i->reg);
}

static void
c_copyrv(InstructionCopyRetV *i, FILE *output)
{
  fprintf(output, "    t[%d] = tr;\n", i->toreg);
}

static void
c_pushreg(InstructionPushReg *i, FILE *output)
{
  fprintf(output, "    stack[++sp] = t[%d];\n", i->reg);
}

static void
c_popreg(InstructionPopReg *i, FILE *output)
{
  fprintf(output, "    t[%d] = stack[sp--];\n", i->reg);
}

static void
c_pcall(InstructionPCall *i, FILE *output)
{
  fprintf(output, "    if (_setjmp(callstack[++csp]) == 0)\n");
  fprintf(output, "        goto label%d;\n", i->label_number);
}

static void
c_fcall(InstructionFCall *i, FILE *output)
{
  fprintf(output, "    if (_setjmp(callstack[++csp]) == 0)\n");
  fprintf(output, "        goto label%d;\n", i->label_number);
}

EmitterWriter *
writer_c_get_emitter(void)
{
  static EmitterWriter w = {
    .write_label	= c_label,
    .write_goto		= c_goto,
    .write_ifnot	= c_ifnot,
    .write_if		= c_if,
    .write_ldimed	= c_ldimed,
    .write_ldreg	= c_ldreg,
    .write_alloc	= c_alloc,
    .write_free		= c_free,
    .write_binop	= c_binop,
    .write_unop		= c_unop,
    .write_load		= c_load,
    .write_store	= c_store,
    .write_read		= c_read,
    .write_write	= c_write,
    .write_return	= c_return,
    .write_returnv	= c_returnv,
    .write_copyrv	= c_copyrv,
    .write_pushreg	= c_pushreg,
    .write_popreg	= c_popreg,
    .write_pcall	= c_pcall,
    .write_fcall	= c_fcall,
    .write_epilog	= c_epilog,
    .write_prolog	= c_prolog
  };
  
  return &w;
}
