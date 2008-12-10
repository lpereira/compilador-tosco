/*
 * Simple Pascal Compiler
 * GUI (User Interface)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __UI_H__
#define __UI_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

typedef enum {
  P_PASS,
  P_TIME,
  P_PERCENTAGE,
  N_PERFORMANCE_COLUMNS
} PerformanceColumns;

typedef struct _UI	UI;

struct _UI {
  GladeXML		*gxml;
  gchar			*filename;
  gfloat		ast_zoom_factor;
  GtkSourceBuffer	*source_buffer;
  GtkListStore		*store_performance;
  GdkPixbuf		*ast_pixbuf;
  GtkWidget		*window,
                        *options_window,
                        *notebook,
                        *scroll_editor,
                        *source_editor,
                        *frame_error,
                        *frame_error_image,
                        *frame_error_label,
                        *frame_error_btn_close,
                        *btn_new,
                        *btn_open,
                        *btn_save,
                        *btn_compile,
                        *btn_execute,
                        *btn_options,
                        *btn_ast,
                        *btn_about,
                        *btn_hide_options,
                        *btn_inczoom,
                        *btn_deczoom,
                        *btn_normzoom,
                        *btn_saveast,
                        *option_cmb_output_language,
                        *option_cmb_style,
                        *option_chk_optl1,
                        *option_chk_optl2,
                        *option_chk_showtime,
                        *option_chk_dupl_avo,
                        *option_spn_tabsize,
                        *option_chk_autoindent,
                        *option_chk_highlightcurrline,
                        *option_chk_showlines,
                        *option_btn_fontchooser,
                        *option_btn_mvd_location,
                        *option_notebook,
                        *ast_img,
                        *tv_performance,
                        *lbl_lncol,
                        *lbl_insovr;
};

UI	*ui_new();
void	 ui_destroy(UI *ui);

#endif	/* __UI_H__ */
