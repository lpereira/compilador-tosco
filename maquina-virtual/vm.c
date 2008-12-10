/*
 * Simple Pascal Compiler
 * Virtual Machine (Machine)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

static void vm_null(VM *vm, VMInstruction *i);
static void vm_ldc(VM *vm, VMInstruction *i);
static void vm_ldv(VM *vm, VMInstruction *i);
static void vm_add(VM *vm, VMInstruction *i);
static void vm_sub(VM *vm, VMInstruction *i);
static void vm_mult(VM *vm, VMInstruction *i);
static void vm_divi(VM *vm, VMInstruction *i);
static void vm_inv(VM *vm, VMInstruction *i);
static void vm_and(VM *vm, VMInstruction *i);
static void vm_or(VM *vm, VMInstruction *i);
static void vm_neg(VM *vm, VMInstruction *i);
static void vm_cme(VM *vm, VMInstruction *i);
static void vm_cma(VM *vm, VMInstruction *i);
static void vm_ceq(VM *vm, VMInstruction *i);
static void vm_cdif(VM *vm, VMInstruction *i);
static void vm_cmeq(VM *vm, VMInstruction *i);
static void vm_cmaq(VM *vm, VMInstruction *i);
static void vm_jmp(VM *vm, VMInstruction *i);
static void vm_jmpf(VM *vm, VMInstruction *i);
static void vm_alloc(VM *vm, VMInstruction *i);
static void vm_dalloc(VM *vm, VMInstruction *i);
static void vm_start(VM *vm, VMInstruction *i);
static void vm_hlt(VM *vm, VMInstruction *i);
static void vm_call(VM *vm, VMInstruction *i);
static void vm_return(VM *vm, VMInstruction *i);
static void vm_returnf(VM *vm, VMInstruction *i);
static void vm_rd(VM *vm, VMInstruction *i);
static void vm_prn(VM *vm, VMInstruction *i);
static void vm_str(VM *vm, VMInstruction *i);

const Instruction instructions[] = {
  { OP_LABEL,	"NULL",		vm_null },
  { OP_LDC,	"LDC",		vm_ldc },
  { OP_LDV,	"LDV", 		vm_ldv },
  { OP_ADD,	"ADD",		vm_add },
  { OP_SUB,	"SUB",		vm_sub },
  { OP_MULT,	"MULT",		vm_mult },
  { OP_DIVI,	"DIVI",		vm_divi },
  { OP_INV,	"INV",		vm_inv },
  { OP_AND,	"AND",		vm_and },
  { OP_OR,	"OR",		vm_or },
  { OP_NEG,	"NEG",		vm_neg },
  { OP_CME,	"CME",		vm_cme },
  { OP_CMA,	"CMA",		vm_cma },
  { OP_CEQ,	"CEQ",		vm_ceq },
  { OP_CDIF,	"CDIF",		vm_cdif },
  { OP_CMEQ,	"CMEQ",		vm_cmeq },
  { OP_CMAQ,	"CMAQ",		vm_cmaq },
  { OP_JMP,	"JMP",		vm_jmp },
  { OP_JMPF,	"JMPF",		vm_jmpf },
  { OP_ALLOC,	"ALLOC",	vm_alloc },
  { OP_DALLOC,	"DALLOC",	vm_dalloc },
  { OP_START,	"START",	vm_start },
  { OP_HLT,	"HLT",		vm_hlt },
  { OP_CALL,	"CALL",		vm_call },
  { OP_RETURN,	"RETURN",	vm_return },
  { OP_RETURNF,	"RETURNF",	vm_returnf },
  { OP_RD,	"RD",		vm_rd },
  { OP_PRN,	"PRN",		vm_prn },
  { OP_STR,	"STR",		vm_str },
};

static gchar *
vm_default_read_function(gpointer data)
{
  static char buffer[128];
  
  printf("<<< ");
  fgets(buffer, 128, stdin);
  return buffer;
}

static void
vm_default_write_function(gpointer data, char *string)
{
  printf(">>> %s\n", string);
}

VM *
vm_new(VMReadFunction read_function, gpointer read_function_data,
       VMWriteFunction write_function, gpointer write_function_data)
{
  VM *vm;
  
  vm = g_new0(VM, 1);

  vm->write_function      = write_function ? write_function : vm_default_write_function;
  vm->read_function       = read_function  ? read_function  : vm_default_read_function;
  vm->write_function_data = write_function_data;
  vm->read_function_data  = read_function_data;
    
  vm_reset(vm);
  
  return vm;
}

void
vm_destroy(VM *vm)
{
  g_free(vm);
}

void
vm_reset(VM *vm)
{
  vm->running = FALSE;
  vm->stack_top = -1;
  vm->instruction_pointer = vm->program;
  
  memset(vm->memory, 0, sizeof(vm->memory));
}

void
vm_object_load(VM *vm, const char *object_file)
{
  GHashTable *label_table;
  GList *list;
  VMInstruction	*instruction;
  gchar buffer[256];
  FILE *object;
  gint line = 0;
  
  vm_object_unload(vm);
  
  label_table = g_hash_table_new(g_str_hash, g_str_equal);
  
  if ((object = fopen(object_file, "r"))) {
    while (fgets(buffer, 256, object)) {
      gchar *label, *instr, *param1, *param2;
      
      if (buffer[0] == ';')
          continue;

      line++;
      
      buffer[3] = '\0';
      buffer[11] = '\0';
      buffer[15] = '\0';
      buffer[19] = '\0';      

      label = g_strchomp(buffer);
      instr = g_strchomp(buffer + 4);
      param1 = g_strchomp(buffer + 12);
      param2 = g_strchomp(buffer + 16);
      
      if (g_str_equal(instr, "NULL")) {		/* label */
        instruction = g_new0(VMInstruction, 1);
        instruction->opcode = OP_LABEL;
        instruction->label_name = g_strdup(label);
        
        g_hash_table_insert(label_table, instruction->label_name,
                            GINT_TO_POINTER(line));
        vm->program = g_list_append(vm->program, instruction);
      } else {					/* everything else */
        int i, valid_instruction = 0;
        
        for (i = 0; i < G_N_ELEMENTS(instructions); i++) {
          if (g_str_equal(instr, instructions[i].name)) {
            instruction = g_new0(VMInstruction, 1);
            instruction->opcode = instructions[i].opcode;
            
            instruction->param1 = atoi(param1);
            instruction->sparam1 = g_strdup(param1);

            instruction->param2 = atoi(param2);
            instruction->sparam2 = g_strdup(param2);

            vm->program = g_list_append(vm->program, instruction);
            
            valid_instruction = 1;
            break;
          }
        }
        
        if (!valid_instruction) {
          g_warning("Ignorando instrução \"%s\" (desconhecida) na linha %d.\n\n"
                    "O programa pode não funcionar corretamente.\n",
                    instr, line);
        }
      }
      
      memset(buffer, '\0', sizeof(buffer));
    }
    
    fclose(object);
  }
  
  for (list = vm->program; list; list = list->next) {
    instruction = (VMInstruction *)list->data;
    
    switch (instruction->opcode) {
      case OP_CALL:
      case OP_JMP:
      case OP_JMPF:
        {
          gchar *label = instruction->sparam1;
          gint label_line = GPOINTER_TO_INT(g_hash_table_lookup(label_table, label));
          
          instruction->sparam2 = g_strdup_printf("<span color=\"#ccc\"><i>Rótulo <b>%s</b></i></span>",
                                                 instruction->sparam1);
          instruction->param1 = label_line;

          g_free(instruction->sparam1);
          instruction->sparam1 = g_strdup_printf("%d", label_line);
        }
        
        break;
      default:
        ;
    }
  }
  
  g_hash_table_destroy(label_table);
  vm_reset(vm);
}

void
vm_object_unload(VM *vm)
{
  VMInstruction *instruction;
  GList *list;
  
  for (list = vm->program; list; list = list->next) {
    instruction = (VMInstruction *)list->data;
    
    g_free(instruction->sparam1);
    g_free(instruction->sparam2);
    g_free(instruction->label_name);
    g_free(instruction);
  }
  
  g_list_free(vm->program);
  vm->program = NULL;
}

void
vm_step(VM *vm)
{
  VMInstruction	*instruction;
  
  if (!vm->program || !vm->instruction_pointer)
    return;
  
  if ((instruction = (VMInstruction *)vm->instruction_pointer->data)) {
    vm->instruction_pointer = vm->instruction_pointer->next;
    
    instructions[instruction->opcode].callback(vm, instruction);
  }
}

static void vm_null(VM *vm, VMInstruction *i)
{
  ;
}
static void vm_ldc(VM *vm, VMInstruction *i)
{
  vm->memory[++vm->stack_top] = i->param1;
}

static void vm_ldv(VM *vm, VMInstruction *i)
{
  vm->memory[++vm->stack_top] = vm->memory[i->param1];
}

static void vm_add(VM *vm, VMInstruction *i)
{
  vm->memory[vm->stack_top - 1] = vm->memory[vm->stack_top - 1] +
                                  vm->memory[vm->stack_top];
  vm->stack_top--;
}

static void vm_sub(VM *vm, VMInstruction *i)
{
  vm->memory[vm->stack_top - 1] = vm->memory[vm->stack_top - 1] -
                                  vm->memory[vm->stack_top];
  vm->stack_top--;
}

static void vm_mult(VM *vm, VMInstruction *i)
{
  vm->memory[vm->stack_top - 1] = vm->memory[vm->stack_top - 1] *
                                  vm->memory[vm->stack_top];
  vm->stack_top--;
}

static void vm_divi(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top] != 0) {
    vm->memory[vm->stack_top - 1] = vm->memory[vm->stack_top - 1] /
                                    vm->memory[vm->stack_top];
    vm->stack_top--;
  } else {
    vm->write_function(vm->write_function_data, "Divisão por zero\n");
    vm->running = FALSE;
    vm->instruction_pointer = NULL;
  }
}

static void vm_inv(VM *vm, VMInstruction *i)
{
  vm->memory[vm->stack_top] = - vm->memory[vm->stack_top];
}

static void vm_and(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] == 1 &&
      vm->memory[vm->stack_top] == 1) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_or(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] == 1 ||
      vm->memory[vm->stack_top] == 1) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_neg(VM *vm, VMInstruction *i)
{
  vm->memory[vm->stack_top] = 1 - vm->memory[vm->stack_top];
}

static void vm_cme(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] < vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_cma(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] > vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_ceq(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] == vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_cdif(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] != vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_cmeq(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] <= vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_cmaq(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top - 1] >= vm->memory[vm->stack_top]) {
    vm->memory[vm->stack_top - 1] = 1;
  } else {
    vm->memory[vm->stack_top - 1] = 0;
  }
  
  vm->stack_top--;
}

static void vm_jmp(VM *vm, VMInstruction *i)
{
  vm->instruction_pointer = g_list_nth(vm->program, i->param1 - 1);
}

static void vm_jmpf(VM *vm, VMInstruction *i)
{
  if (vm->memory[vm->stack_top] == 0) {
    vm_jmp(vm, i);
  }
  
  vm->stack_top--;
}

static void vm_alloc(VM *vm, VMInstruction *i)
{
  int k;
  
  for (k = 0; k <= i->param2 - 1; k++) {
    vm->stack_top++;
    vm->memory[vm->stack_top] = vm->memory[i->param1 + k];
  }
}

static void vm_dalloc(VM *vm, VMInstruction *i)
{
  int k;
  
  for (k = i->param2 - 1; k >= 0; k--) {
    vm->memory[i->param1 + k] = vm->memory[vm->stack_top];
    vm->stack_top--;
  }
}

static void vm_start(VM *vm, VMInstruction *i)
{
  vm->stack_top = -1;
}

static void vm_hlt(VM *vm, VMInstruction *i)
{
  vm->running = FALSE;
}

static void vm_call(VM *vm, VMInstruction *i)
{
  vm->stack_top++;
  vm->memory[vm->stack_top] = g_list_position(vm->program,
                                              vm->instruction_pointer) + 1;
  vm_jmp(vm, i);
}

static void vm_return(VM *vm, VMInstruction *i)
{
  vm->instruction_pointer = g_list_nth(vm->program, vm->memory[vm->stack_top] - 1);
  vm->stack_top--;
}

static void vm_returnf(VM *vm, VMInstruction *i)
{
  gint return_value;
  
  return_value = vm->memory[i->param1];
  
  i->param2 = 1;
  vm_dalloc(vm, i);
  vm_return(vm, i);

  vm->memory[++vm->stack_top] = return_value;
}

static void vm_rd(VM *vm, VMInstruction *i)
{
  vm->stack_top++;
  vm->memory[vm->stack_top] = atoi(vm->read_function(vm->read_function_data));
}

static void vm_prn(VM *vm, VMInstruction *i)
{
  gchar *tmp;
  
  tmp = g_strdup_printf("%d", vm->memory[vm->stack_top]);
  vm->write_function(vm->write_function_data, tmp);
  vm->stack_top--;
  
  g_free(tmp);
}

static void vm_str(VM *vm, VMInstruction *i)
{
  vm->memory[i->param1] = vm->memory[vm->stack_top];
  vm->stack_top--;
}

