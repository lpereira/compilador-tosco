/*
 * Simple Pascal Compiler
 * GTK+ Treeview Helper
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

#include <gtk/gtk.h>

typedef struct _TreeViewColumn TreeViewColumn;

typedef enum {
  RENDERER_PIXBUF,
  RENDERER_TEXT,
  RENDERER_PERCENTAGE
} TreeViewRendererType;

struct _TreeViewColumn {
  gchar			*header_label;
  TreeViewRendererType	renderer_type;
  gint			store_column;
};

void tree_view_add_columns(GtkTreeView *tv, TreeViewColumn *columns, gint n);

#endif	/* __TREEVIEW_H__ */
