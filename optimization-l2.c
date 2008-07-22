#include "optimization-l2.h"
#include "codeemitter.h"

/*
 * Possible optimizations
 *
 * Easy:
 * - Remove "label n: goto label m;" 
 * - Change "tx := not tx; if_not tx (...)" to "if tx (...)"
 *
 * Difficult:
 * - Common subexpression elimination
 */

/*
 * This routine tries to optimize unnecessary memory access, by changing
 * code such as:
 *
 *      store <memory position>, <register>
 *      load <register>, <memory position>
 *
 * To:
 *
 *      store <memory position>, <register>
 */
static void
optimize_load_store(GList *code)
{
  GList *l;
  Instruction *curr, *next;
  
  for (l = code; l && l->next; l = l->next) {
    curr = (Instruction *)l->data;
    next = (Instruction *)l->next->data;
    
    if (curr->type == I_STORE && next->type == I_LOAD) {
      InstructionStore *store = (InstructionStore *)&curr->params;
      InstructionLoad *load = (InstructionLoad *)&next->params;
      
      if (store->reg == load->reg &&
          store->offset == load->offset &&
          store->size == load->size) {
        GList *next_node = l->next->next;
        
        g_free(l->next->data);
        g_list_free_1(l->next);
        l->next = next_node;
        l = l->next;
      }
    }
  }
}

void
optimization_l2(Emitter *emitter)
{
  optimize_load_store(emitter->code);
}
