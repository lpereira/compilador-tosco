/*
 * Simple Pascal Compiler
 * Virtual Machine (Machine)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __VM_H__
#define __VM_H__

#include <glib.h>

typedef gchar *	(*VMReadFunction)	(gpointer data);
typedef void	(*VMWriteFunction)	(gpointer data, char *string);

typedef struct _VM		VM;
typedef struct _VMInstruction	VMInstruction;
typedef struct _Instruction	Instruction;

typedef enum {
  OP_LABEL,
  OP_LDC,
  OP_LDV,
  OP_ADD,
  OP_SUB,
  OP_MULT,
  OP_DIVI,
  OP_INV,
  OP_AND,
  OP_OR,
  OP_NEG,
  OP_CME,
  OP_CMA,
  OP_CEQ,
  OP_CDIF,
  OP_CMEQ,
  OP_CMAQ,
  OP_JMP,
  OP_JMPF,
  OP_ALLOC,
  OP_DALLOC,
  OP_START,
  OP_HLT,
  OP_CALL,
  OP_RETURN,
  OP_RETURNF,
  OP_RD,
  OP_PRN,
  OP_STR,
  N_OP
} VMOpcode;

struct _Instruction {
  VMOpcode	 opcode;
  char		*name;
  void		(* callback)(VM *vm, VMInstruction *i);
};

struct _VM {
  int			stack_top;
  gint			memory[65536];
  GList			*program, *instruction_pointer;
  gboolean		running;
  
  VMReadFunction	read_function;
  VMWriteFunction	write_function;
  gpointer		write_function_data, read_function_data;
};

struct _VMInstruction {
  VMOpcode	 opcode;
  guint		 param1, param2;
  gchar		*sparam1, *sparam2;
  gchar		*label_name;
  gpointer	 data;
};

extern const Instruction instructions[];

VM 	*vm_new(VMReadFunction read_function, gpointer read_function_data,
                VMWriteFunction write_function, gpointer write_function_data);
void	 vm_destroy(VM *vm);

void	 vm_object_load(VM *vm, const char *object_file);
void	 vm_object_unload(VM *vm);

void	 vm_step(VM *vm);
void	 vm_reset(VM *vm);

#endif	/* __VM_H__ */
