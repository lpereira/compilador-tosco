/*
 * Simple Pascal Compiler
 * GTK+ Treeview Helper
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include "treeview.h"

static const void *treeview_renderers[] = { gtk_cell_renderer_pixbuf_new, gtk_cell_renderer_text_new };
static const char *treeview_attributes[] = { "pixbuf", "markup" };

void tree_view_add_columns(GtkTreeView *tv, TreeViewColumn *columns, gint n)
{
  int i;
  
  for (i = 0; i < n; i++) {
    GtkTreeViewColumn	*tvcolumn;
    TreeViewColumn	*column = columns + i;
    GtkCellRenderer	*(*renderer)(void);
    const char		*attribute;
    
    attribute = treeview_attributes[column->renderer_type];
    renderer = treeview_renderers[column->renderer_type];

    tvcolumn = gtk_tree_view_column_new_with_attributes(column->header_label,
                                                        renderer(),
                                                        attribute, column->store_column,
                                                        NULL);
    gtk_tree_view_append_column(tv, tvcolumn);
  }
}

