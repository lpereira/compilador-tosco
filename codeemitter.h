#ifndef __CODEEMITTER_H__
#define __CODEEMITTER_H__

#include <glib.h>
#include <stdio.h>

typedef struct _Emitter			Emitter;
typedef struct _EmitterWriter		EmitterWriter;

typedef struct _Instruction		Instruction;

typedef struct _InstructionLabel	InstructionLabel;
typedef struct _InstructionGoto		InstructionGoto;
typedef struct _InstructionIfNot	InstructionIfNot;
typedef struct _InstructionIf		InstructionIf;
typedef struct _InstructionLdImed	InstructionLdImed;
typedef struct _InstructionLdReg	InstructionLdReg;
typedef struct _InstructionAlloc	InstructionAlloc;
typedef struct _InstructionFree		InstructionFree;
typedef struct _InstructionBinOp	InstructionBinOp;
typedef struct _InstructionUnOp		InstructionUnOp;
typedef struct _InstructionLoad		InstructionLoad;
typedef struct _InstructionStore	InstructionStore;
typedef struct _InstructionRead		InstructionRead;
typedef struct _InstructionWrite	InstructionWrite;
typedef struct _InstructionReturn	InstructionReturn;
typedef struct _InstructionReturnV	InstructionReturnV;
typedef struct _InstructionCopyRetV	InstructionCopyRetV;
typedef struct _InstructionPushReg	InstructionPushReg;
typedef struct _InstructionPopReg	InstructionPopReg;
typedef struct _InstructionPCall	InstructionPCall;
typedef struct _InstructionFCall	InstructionFCall;

typedef enum {
  I_LABEL,
  I_GOTO,
  I_IFNOT,
  I_IF,
  I_LDIMED,
  I_LDREG,
  I_ALLOC,
  I_FREE,
  I_BINOP,
  I_UNOP,
  I_LOAD,
  I_STORE,
  I_READ,
  I_WRITE,
  I_RETURN,
  I_RETURNV,
  I_PUSHREG,
  I_POPREG,
  I_PCALL,
  I_FCALL
} InstructionType;

struct _Emitter {
  GList	*code;
};

struct _InstructionLabel {
  gint number;
};

struct _InstructionGoto {
  gint label;
};

struct _InstructionIfNot {
  guint reg;
  guint label;
};

struct _InstructionIf {
  guint reg;
  guint label;
};

struct _InstructionLdImed {
  guint reg;
  guint value;
};

struct _InstructionLdReg {
  guint reg1, reg2;
};

struct _InstructionAlloc {
  guint bytes;
};

struct _InstructionFree {
  guint bytes;
};

struct _InstructionBinOp {
  guint reg1, reg2, result;
  gchar *op;	/* FIXME */
};

struct _InstructionUnOp {
  guint reg, result;
  gchar *op;	/* FIXME */
};

struct _InstructionLoad {
  guint reg;
  guint offset;
  guint size;
};

struct _InstructionStore {
  guint reg;
  guint offset;
  guint size;
};

struct _InstructionRead {
  guint reg;
};

struct _InstructionWrite {
  guint reg;
};

struct _InstructionReturn {
};

struct _InstructionReturnV {
  guint reg;
};

struct _InstructionCopyRetV {
  guint toreg;
};

struct _InstructionPushReg {
  guint reg;
};

struct _InstructionPopReg {
  guint reg;
};

struct _InstructionPCall {
  guint label_number;
};

struct _InstructionFCall {
  guint label_number;
};

struct _Instruction {
  InstructionType	type;
  union {
    InstructionLabel	p_label;
    InstructionGoto	p_goto;
    InstructionIfNot	p_ifnot;
    InstructionIf	p_if;
    InstructionLdImed	p_ldimed;
    InstructionLdReg	p_ldreg;
    InstructionAlloc	p_alloc;
    InstructionFree	p_free;
    InstructionBinOp	p_binop;
    InstructionUnOp	p_unop;
    InstructionLoad	p_load;
    InstructionStore	p_store;
    InstructionRead	p_read;
    InstructionWrite	p_write;
    InstructionReturn	p_return;
    InstructionReturnV	p_returnv;
    InstructionCopyRetV p_copyretv;
    InstructionPushReg	p_pushreg;
    InstructionPopReg	p_popreg;
    InstructionPCall	p_pcall;
    InstructionFCall	p_fcall;
  } params;
};

struct _EmitterWriter {
  void	(*write_label)	(InstructionLabel i);
  void	(*write_goto)	(InstructionGoto i);
  void	(*write_ifnot)	(InstructionIfNot i);
  void	(*write_if)	(InstructionIf i);
  void	(*write_ldimed)	(InstructionLdImed i);
  void	(*write_ldreg)	(InstructionLdReg i);
  void	(*write_alloc)	(InstructionAlloc i);
  void	(*write_free)	(InstructionFree i);
  void	(*write_binop)	(InstructionBinOp i);
  void	(*write_unop)	(InstructionUnOp i);
  void	(*write_load)	(InstructionLoad i);
  void	(*write_store)	(InstructionStore i);
  void	(*write_read)	(InstructionRead i);
  void	(*write_write)	(InstructionWrite i);
  void	(*write_return)	(InstructionReturn i);
  void	(*write_returnv)(InstructionReturnV i);
  void	(*write_copyrv)	(InstructionCopyRetV i);
  void	(*write_pushreg)(InstructionPushReg i);
  void	(*write_popreg)	(InstructionPopReg i);
  void	(*write_pcall)	(InstructionPCall i);
  void	(*write_fcall)	(InstructionFCall i);
  
  void	(*write_prolog)	(void);
  void	(*write_epilog)	(void);
};

Emitter	*emitter_new();
void	 emitter_write(Emitter *emitter, EmitterWriter *writer, FILE *output);
void	 emitter_destroy(Emitter *emitter);

void	 emitter_emit_label(Emitter *emitter, guint number);
void	 emitter_emit_goto(Emitter *emitter, guint label_number);
void	 emitter_emit_ifnot(Emitter *emitter, guint reg, guint label);
void	 emitter_emit_if(Emitter *emitter, guint reg, guint label);
void	 emitter_emit_ldimed(Emitter *emitter, guint reg, guint value);
void	 emitter_emit_ldreg(Emitter *emitter, guint reg1, guint reg2);
void	 emitter_emit_alloc(Emitter *emitter, guint bytes);
void	 emitter_emit_free(Emitter *emitter, guint bytes);
void	 emitter_emit_binop(Emitter *emitter, guint result, guint reg1, const gchar *op, guint reg2);
void	 emitter_emit_unop(Emitter *emitter, guint result, const gchar *op, guint reg);
void	 emitter_emit_load(Emitter *emitter, guint reg, guint offset, guint size);
void	 emitter_emit_store(Emitter *emitter, guint reg, guint offset, guint size);
void	 emitter_emit_read(Emitter *emitter, guint reg);
void	 emitter_emit_write(Emitter *emitter, guint reg);
void	 emitter_emit_return(Emitter *emitter);
void	 emitter_emit_return_value(Emitter *emitter, guint reg);
void	 emitter_emit_copy_return_value(Emitter *emitter, guint toreg);
void	 emitter_emit_pushreg(Emitter *emitter, guint reg);
void	 emitter_emit_popreg(Emitter *emitter, guint reg);
void	 emitter_emit_pcall(Emitter *emitter, guint label_number);
void	 emitter_emit_fcall(Emitter *emitter, guint label_number);

#endif /* __CODEEMITTER_H__ */
