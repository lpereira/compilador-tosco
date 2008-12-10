/*
 * Simple Pascal Compiler
 * Virtual Machine (User Interface)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __UI_H__
#define __UI_H__

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "vm.h"

typedef struct _UI	UI;

typedef enum {
  IC_POINTER,
  IC_LABEL,
  IC_NAME,
  IC_PARAM1,
  IC_PARAM2,
  N_INSTRUCTION_COLUMNS
} InstructionColumns;

typedef enum {
  MC_ADDRESS,
  MC_CONTENT,
  N_MEMORY_COLUMNS
} MemoryColumns;

struct _UI {
  VM		*vm;
  GladeXML	*gxml;
  GtkWidget	*window,
                *notebook,
                *tv_instructions,
                *tv_memory,
                *output,
                *input_box,
                *input,
                *input_enter,
                *btn_open,
                *btn_execute,
                *btn_step_by_step,
                *btn_reset,
                *btn_about;
  GtkListStore	*store_instructions,
                *store_memory;
  GdkPixbuf	*pbuf_arrow;
  gboolean	selection_changeable;
};

UI	*ui_new(char *object_file);
void	 ui_destroy(UI *ui);

#endif	/* __UI_H__ */
