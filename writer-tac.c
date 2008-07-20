#include "writer-tac.h"

static void
tac_epilog(FILE *output)
{
  ;
}

static void
tac_prolog(FILE *output)
{
  ;
}

static void
tac_label(InstructionLabel *i, FILE *output)
{
  fprintf(output, "label%d:\n", i->number);
}

static void
tac_goto(InstructionGoto *i, FILE *output)
{
  fprintf(output, "    goto label%d\n", i->label);
}

static void
tac_ifnot(InstructionIfNot *i, FILE *output)
{
  fprintf(output, "    if_not t%d goto label%d\n", i->reg, i->label);
}

static void
tac_if(InstructionIf *i, FILE *output)
{
  fprintf(output, "    if t%d goto label%d\n", i->reg, i->label);
}

static void
tac_ldimed(InstructionLdImed *i, FILE *output)
{
  fprintf(output, "    t%d := %d\n", i->reg, i->value);
}

static void
tac_ldreg(InstructionLdReg *i, FILE *output)
{
  fprintf(output, "    t%d := t%d\n", i->reg1, i->reg2);
}

static void
tac_alloc(InstructionAlloc *i, FILE *output)
{
  fprintf(output, "    mp := mp + %d\n", i->bytes);
}

static void
tac_free(InstructionFree *i, FILE *output)
{
  fprintf(output, "    mp := mp - %d\n", i->bytes);
}

static void
tac_binop(InstructionBinOp *i, FILE *output)
{
  fprintf(output, "    t%d := t%d %s t%d\n", i->result, i->reg1, i->op, i->reg2);
}

static void
tac_unop(InstructionUnOp *i, FILE *output)
{
  fprintf(output, "    t%d := %s t%d\n", i->result, i->op, i->reg);
}

static void
tac_load(InstructionLoad *i, FILE *output)
{
  fprintf(output, "    load t%d, mp-%d, %d\n", i->reg, i->offset, i->size);
}

static void
tac_store(InstructionStore *i, FILE *output)
{
  fprintf(output, "    store mp-%d, %d, t%d\n", i->offset, i->size, i->reg);
}

static void
tac_read(InstructionRead *i, FILE *output)
{
  fprintf(output, "    read t%d\n", i->reg);
}

static void
tac_write(InstructionWrite *i, FILE *output)
{
  fprintf(output, "    write t%d\n", i->reg);
}

static void
tac_return(InstructionReturn *i, FILE *output)
{
  fprintf(output, "    return\n");
}

static void
tac_returnv(InstructionReturnV *i, FILE *output)
{
  fprintf(output, "    tr := t%d\n", i->reg);
}

static void
tac_copyrv(InstructionCopyRetV *i, FILE *output)
{
  fprintf(output, "    t%d := tr\n", i->toreg);
}

static void
tac_pushreg(InstructionPushReg *i, FILE *output)
{
  fprintf(output, "    push t%d\n", i->reg);
}

static void
tac_popreg(InstructionPopReg *i, FILE *output)
{
  fprintf(output, "    pop t%d\n", i->reg);
}

static void
tac_pcall(InstructionPCall *i, FILE *output)
{
  fprintf(output, "    call label%d\n", i->label_number);
}

static void
tac_fcall(InstructionFCall *i, FILE *output)
{
  fprintf(output, "    call label%d\n", i->label_number);
}

EmitterWriter *
writer_tac_get_emitter(void)
{
  static EmitterWriter w = {
    .write_label	= tac_label,
    .write_goto		= tac_goto,
    .write_ifnot	= tac_ifnot,
    .write_if		= tac_if,
    .write_ldimed	= tac_ldimed,
    .write_ldreg	= tac_ldreg,
    .write_alloc	= tac_alloc,
    .write_free		= tac_free,
    .write_binop	= tac_binop,
    .write_unop		= tac_unop,
    .write_load		= tac_load,
    .write_store	= tac_store,
    .write_read		= tac_read,
    .write_write	= tac_write,
    .write_return	= tac_return,
    .write_returnv	= tac_returnv,
    .write_copyrv	= tac_copyrv,
    .write_pushreg	= tac_pushreg,
    .write_popreg	= tac_popreg,
    .write_pcall	= tac_pcall,
    .write_fcall	= tac_fcall,
    .write_epilog	= tac_epilog,
    .write_prolog	= tac_prolog
  };
  
  return &w;
}
