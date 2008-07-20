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
  
}

static void
tac_goto(InstructionGoto *i, FILE *output)
{

}

static void
tac_ifnot(InstructionIfNot *i, FILE *output)
{

}

static void
tac_if(InstructionIf *i, FILE *output)
{

}

static void
tac_ldimed(InstructionLdImed *i, FILE *output)
{

}

static void
tac_ldreg(InstructionLdReg *i, FILE *output)
{

}

static void
tac_alloc(InstructionAlloc *i, FILE *output)
{

}

static void
tac_free(InstructionFree *i, FILE *output)
{

}

static void
tac_binop(InstructionBinOp *i, FILE *output)
{

}

static void
tac_unop(InstructionUnOp *i, FILE *output)
{

}

static void
tac_load(InstructionLoad *i, FILE *output)
{

}

static void
tac_store(InstructionStore *i, FILE *output)
{

}

static void
tac_read(InstructionRead *i, FILE *output)
{

}

static void
tac_write(InstrutionWrite *i, FILE *output)
{

}

static void
tac_return(InstructionReturn *i, FILE *output)
{

}

static void
tac_returnv(InstructionReturnV *i, FILE *output)
{

}

static void
tac_copyrv(InstructionCopyRetV *i, FILE *output)
{

}

static void
tac_pushreg(InstructionPushReg *i, FILE *output)
{

}

static void
tac_popreg(InstructionPopReg *i, FILE *output)
{

}

static void
tac_pcall(InstructionPCall *i, FILE *output)
{

}

static void
tac_fcall(InstructionFCall *i, FILE *output)
{

}

EmitterWriter *
writer_tac_get_emitter(void)
{
  return NULL;
}
