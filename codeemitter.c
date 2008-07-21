/*
 * Simple Pascal Compiler
 * Code Emitter / Writer
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include <string.h>

#include "codeemitter.h"

Emitter        *
emitter_new()
{
	Emitter        *emitter;

	emitter = g_new0(Emitter, 1);
	emitter->code = NULL;

	return emitter;
}

void
emitter_free(Emitter * emitter)
{
	g_list_free(emitter->code);
	g_free(emitter);
}

void
emitter_write(Emitter * emitter,
	      EmitterWriter * writer,
	      FILE * output)
{
	GList          *inst;

	g_return_if_fail(emitter);
	g_return_if_fail(writer);
	g_return_if_fail(output);

	writer->write_prolog(output);

	for (inst = emitter->code; inst; inst = inst->next) {
		Instruction    *instruction = (Instruction *) inst->data;
		gpointer	params = &(instruction->params);

		switch (instruction->type) {
		case I_LABEL:
			writer->write_label((InstructionLabel *)params, output);
			break;
		case I_GOTO:
			writer->write_goto((InstructionGoto *)params, output);
			break;
		case I_IFNOT:
			writer->write_ifnot((InstructionIfNot *)params, output);
			break;
		case I_IF:
			writer->write_if((InstructionIf *)params, output);
			break;
		case I_LDIMED:
			writer->write_ldimed((InstructionLdImed *)params, output);
			break;
		case I_LDREG:
			writer->write_ldreg((InstructionLdReg *)params, output);
			break;
		case I_ALLOC:
			writer->write_alloc((InstructionAlloc *)params, output);
			break;
		case I_FREE:
			writer->write_free((InstructionFree *)params, output);
			break;
		case I_BINOP:
			writer->write_binop((InstructionBinOp *)params, output);
			break;
		case I_UNOP:
			writer->write_unop((InstructionUnOp *)params, output);
			break;
		case I_LOAD:
			writer->write_load((InstructionLoad *)params, output);
			break;
		case I_STORE:
			writer->write_store((InstructionStore *)params, output);
			break;
		case I_READ:
			writer->write_read((InstructionRead *)params, output);
			break;
		case I_WRITE:
			writer->write_write((InstructionWrite *)params, output);
			break;
		case I_RETURN:
			writer->write_return((InstructionReturn *)params, output);
			break;
		case I_RETURNV:
			writer->write_returnv((InstructionReturnV *)params, output);
			break;
		case I_COPYRETV:
			writer->write_copyrv((InstructionCopyRetV *)params, output);
			break;
		case I_PUSHREG:
			writer->write_pushreg((InstructionPushReg *)params, output);
			break;
		case I_POPREG:
			writer->write_popreg((InstructionPopReg *)params, output);
			break;
		case I_PCALL:
			writer->write_pcall((InstructionPCall *)params, output);
			break;
		case I_FCALL:
			writer->write_fcall((InstructionFCall *)params, output);
			break;
		default:
			g_error("Don't know how to emit instruction %d.", instruction->type);
		}
	}

	writer->write_epilog(output);
}

static void
emitter_emit_instruction(Emitter * emitter,
			 InstructionType type,
			 gpointer instruction,
			 size_t instruction_size)
{
	Instruction    *i = g_new0(Instruction, 1);

	i->type = type;
	memcpy(&(i->params), instruction, instruction_size);

	emitter->code = g_list_append(emitter->code, i);
}

void
emitter_emit_label(Emitter * emitter, guint number)
{
	InstructionLabel i_label;

	i_label.number = number;

	emitter_emit_instruction(emitter, I_LABEL, &i_label, sizeof(i_label));
}

void
emitter_emit_goto(Emitter * emitter, guint label_number)
{
	InstructionGoto i_goto;

	i_goto.label = label_number;

	emitter_emit_instruction(emitter, I_GOTO, &i_goto, sizeof(i_goto));
}

void
emitter_emit_ifnot(Emitter * emitter, guint reg, guint label)
{
	InstructionIfNot i_ifnot;

	i_ifnot.reg = reg;
	i_ifnot.label = label;

	emitter_emit_instruction(emitter, I_IFNOT, &i_ifnot, sizeof(i_ifnot));
}

void
emitter_emit_if(Emitter * emitter, guint reg, guint label)
{
	InstructionIf   i_if;

	i_if.reg = reg;
	i_if.label = label;

	emitter_emit_instruction(emitter, I_IF, &i_if, sizeof(i_if));
}

void
emitter_emit_ldimed(Emitter * emitter, guint reg, guint value)
{
	InstructionLdImed i_ldimed;

	i_ldimed.reg = reg;
	i_ldimed.value = value;

	emitter_emit_instruction(emitter, I_LDIMED, &i_ldimed, sizeof(i_ldimed));
}

void
emitter_emit_ldreg(Emitter * emitter, guint reg1, guint reg2)
{
	InstructionLdReg i_ldreg;

	i_ldreg.reg1 = reg1;
	i_ldreg.reg2 = reg2;

	emitter_emit_instruction(emitter, I_LDREG, &i_ldreg, sizeof(i_ldreg));
}

void
emitter_emit_alloc(Emitter * emitter, guint bytes)
{
	InstructionAlloc i_alloc;

	i_alloc.bytes = bytes;

	emitter_emit_instruction(emitter, I_ALLOC, &i_alloc, sizeof(i_alloc));
}

void
emitter_emit_free(Emitter * emitter, guint bytes)
{
	InstructionFree i_free;

	i_free.bytes = bytes;

	emitter_emit_instruction(emitter, I_FREE, &i_free, sizeof(i_free));
}

void
emitter_emit_binop(Emitter * emitter,
		   guint result,
		   guint reg1, const gchar * op, guint reg2)
{
	InstructionBinOp i_binop;

	i_binop.result = result;
	i_binop.reg1 = reg1;
	i_binop.op = (gchar *) op;
	i_binop.reg2 = reg2;

	emitter_emit_instruction(emitter, I_BINOP, &i_binop, sizeof(i_binop));
}

void
emitter_emit_unop(Emitter * emitter,
		  guint result,
		  const gchar * op, guint reg)
{
	InstructionUnOp i_unop;

	i_unop.result = result;
	i_unop.op = (gchar *) op;
	i_unop.reg = reg;

	emitter_emit_instruction(emitter, I_UNOP, &i_unop, sizeof(i_unop));
}

void
emitter_emit_load(Emitter * emitter, guint reg, guint offset, guint size)
{
	InstructionLoad i_load;

	i_load.reg = reg;
	i_load.offset = offset;
	i_load.size = size;

	emitter_emit_instruction(emitter, I_LOAD, &i_load, sizeof(i_load));
}

void
emitter_emit_store(Emitter * emitter, guint reg, guint offset, guint size)
{
	InstructionStore i_store;

	i_store.reg = reg;
	i_store.offset = offset;
	i_store.size = size;

	emitter_emit_instruction(emitter, I_STORE, &i_store, sizeof(i_store));
}

void
emitter_emit_read(Emitter * emitter, guint reg)
{
	InstructionRead i_read;

	i_read.reg = reg;

	emitter_emit_instruction(emitter, I_READ, &i_read, sizeof(i_read));
}

void
emitter_emit_write(Emitter * emitter, guint reg)
{
	InstructionWrite i_write;

	i_write.reg = reg;

	emitter_emit_instruction(emitter, I_WRITE, &i_write, sizeof(i_write));
}

void
emitter_emit_return(Emitter * emitter)
{
	InstructionReturn i_return;

	emitter_emit_instruction(emitter, I_RETURN, &i_return, sizeof(i_return));
}

void
emitter_emit_return_value(Emitter * emitter, guint reg)
{
	InstructionReturnV i_returnv;

	i_returnv.reg = reg;

	emitter_emit_instruction(emitter, I_RETURNV, &i_returnv, sizeof(i_returnv));
}

void
emitter_emit_copy_return_value(Emitter * emitter, guint toreg)
{
	InstructionCopyRetV i_copyretv;

	i_copyretv.toreg = toreg;

	emitter_emit_instruction(emitter, I_COPYRETV, &i_copyretv, sizeof(i_copyretv));
}

void
emitter_emit_pushreg(Emitter * emitter, guint reg)
{
	InstructionPushReg i_pushreg;

	i_pushreg.reg = reg;

	emitter_emit_instruction(emitter, I_PUSHREG, &i_pushreg, sizeof(i_pushreg));
}

void
emitter_emit_popreg(Emitter * emitter, guint reg)
{
	InstructionPopReg i_popreg;

	i_popreg.reg = reg;

	emitter_emit_instruction(emitter, I_POPREG, &i_popreg, sizeof(i_popreg));
}

void
emitter_emit_pcall(Emitter * emitter, guint label_number)
{
	InstructionPCall i_pcall;

	i_pcall.label_number = label_number;

	emitter_emit_instruction(emitter, I_PCALL, &i_pcall, sizeof(i_pcall));
}

void
emitter_emit_fcall(Emitter * emitter, guint label_number)
{
	InstructionFCall i_fcall;

	i_fcall.label_number = label_number;

	emitter_emit_instruction(emitter, I_FCALL, &i_fcall, sizeof(i_fcall));
}
