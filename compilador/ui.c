/*
 * Simple Pascal Compiler
 * GUI (User Interface)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdlib.h>

#include "conf.h"
#include "ui.h"
#include "treeview.h"

#include "lpd_lang.h"
#include "compiler_glade.h"
#include "hammer.h"

#include "compiler_main.h"

static const char __PROGRAM_NAME__[] = "Compilador Simplificado Didático";
static const char __PROGRAM_VERSION__[] = "1.0";

static void cb_btn_new_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_open_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_save_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_compile_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_options_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_ast_clicked(GtkWidget * widget, gpointer data);
static void cb_btn_about_clicked(GtkWidget * widget, gpointer data);

void ui_set_error(UI * ui, gchar * msg, ...);
void ui_set_message(UI * ui, gchar * msg, ...);
void ui_show_performance(UI *ui, gchar *performance_output);

static void ui_scroll_to_line(UI *ui, gint line, gint column, char *search_token)
{
    GtkTextIter		iter, mstart, mend;
    gboolean		found;
    
    if (column == 0)
      column = 1;
    
    if (line == 0)
      line = 1;
    
    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(ui->source_buffer),
                                     &iter, line - 1);

    if (!gtk_text_iter_forward_chars(&iter, column - 1)) {
        gtk_text_iter_backward_chars(&iter, column - 1);
    }
    
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ui->source_editor), &iter, 0.1f, FALSE, 0.0f, 0.0f);
    gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(ui->source_buffer), &iter);
    
    if (search_token) {
        found = gtk_text_iter_forward_search(&iter, search_token, 0,
                                             &mstart, &mend, NULL);
      
        if (found) {
            gtk_text_buffer_select_range(GTK_TEXT_BUFFER(ui->source_buffer),
                                         &mstart, &mend);
        }
    }
    
    gtk_widget_grab_focus(ui->source_editor);
}

static void ui_scroll_to_error(UI *ui, gchar *error_message)
{
    gint		line, column;
    char		token[512];
    
    if (sscanf(error_message, "Erro: ln <b>%d</b>, col <b>%d</b>, próximo a <b>%s </b>",
               &line, &column, token) == 3) {
      ui_scroll_to_line(ui, line, column, token);
    } else if (sscanf(error_message, "Erro: ln <b>%d</b>, col <b>%d</b>", &line, &column) == 2) {
      ui_scroll_to_line(ui, line, column, NULL);
    }
}

static void ui_reset(UI *ui)
{
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(ui->source_buffer), "", -1);
    
    gtk_widget_set_sensitive(ui->btn_compile, FALSE);
    gtk_widget_set_sensitive(ui->btn_execute, FALSE);
    gtk_widget_set_sensitive(ui->btn_ast, FALSE);
    gtk_widget_set_sensitive(ui->btn_save, FALSE);
    
    ui_set_error(ui, NULL);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui->btn_ast), FALSE);
    
    ui_show_performance(ui, NULL);
}

static void ui_save(UI * ui)
{
    GtkTextIter		start_iter, end_iter;
    gchar		*source_code;

    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(ui->source_buffer),
				   &start_iter);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(ui->source_buffer),
				 &end_iter);

    source_code =
	gtk_text_buffer_get_text(GTK_TEXT_BUFFER(ui->source_buffer),
				 &start_iter, &end_iter, TRUE);
    g_file_set_contents(ui->filename, source_code, -1, NULL);

    gtk_widget_set_sensitive(ui->btn_save, FALSE);
    gtk_widget_set_sensitive(ui->btn_compile, TRUE);
    gtk_widget_set_sensitive(ui->btn_ast, TRUE);
  
    g_free(source_code);
}

static void ui_open(UI * ui, gchar * filename)
{
    gchar		*contents;
    
    ui_reset(ui);
    g_free(ui->filename);
    
    ui->filename = g_strdup(filename);
    g_file_get_contents(ui->filename, &contents, NULL, NULL);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(ui->source_buffer), contents, -1);

    gtk_widget_set_sensitive(ui->btn_compile, TRUE);
    gtk_widget_set_sensitive(ui->btn_ast, TRUE);
    gtk_widget_set_sensitive(ui->btn_save, FALSE);
    
    g_free(contents);
}

static void cb_window_destroy(GtkWidget * widget, gpointer data)
{
    gtk_main_quit();
}

static void ui_verify_modified_state(UI * ui)
{
    if (GTK_WIDGET_SENSITIVE(ui->btn_save)) {
	GtkWidget	*dialog;

	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_NONE,
					"Existem mudanças não salvas neste arquivo. Salvar as mudanças?");
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       GTK_STOCK_NO, GTK_RESPONSE_REJECT,
			       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	    cb_btn_save_clicked(NULL, ui);
	}

	gtk_widget_destroy(dialog);
    }
}

static void cb_btn_new_clicked(GtkWidget * widget, gpointer data)
{
    UI			*ui = (UI *) data;

    ui_verify_modified_state(ui);
    ui_reset(ui);

    gtk_widget_set_sensitive(ui->btn_save, FALSE);
    gtk_window_set_title(GTK_WINDOW(ui->window), __PROGRAM_NAME__);

    g_free(ui->filename);
    ui->filename = NULL;
}

static void cb_btn_open_clicked(GtkWidget * widget, gpointer data)
{
    UI			*ui = (UI *) data;
    GtkFileFilter	*filter;
    GtkWidget		*dialog;

    ui_verify_modified_state(ui);

    dialog = gtk_file_chooser_dialog_new("Abrir Fonte",
					 GTK_WINDOW(ui->window),
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN,
					 GTK_RESPONSE_ACCEPT, NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.[Ll][Pp][Dd]");
    gtk_file_filter_set_name(filter, "Arquivos Fonte (*.lpd)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename, *short_filename;
	char *title;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	short_filename = g_path_get_basename(filename);
	title = g_strdup_printf("%s - %s", short_filename, __PROGRAM_NAME__);
	gtk_window_set_title(GTK_WINDOW(ui->window), title);

	ui_open(ui, filename);

	g_free(filename);
	g_free(short_filename);
	g_free(title);
    }

    gtk_widget_destroy(dialog);
}

static void cb_btn_save_clicked(GtkWidget * widget, gpointer data)
{
    UI			*ui = (UI *) data;
    GtkFileFilter	*filter;
    GtkWidget		*dialog;

    if (ui->filename) {
	ui_save(ui);
	return;
    }

    dialog = gtk_file_chooser_dialog_new("Salvar Fonte",
					 GTK_WINDOW(ui->window),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE,
					 GTK_RESPONSE_ACCEPT, NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.[Ll][Pp][Dd]");
    gtk_file_filter_set_name(filter, "Arquivos Fonte (*.lpd)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename, *short_filename;
	char *title;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	short_filename = g_path_get_basename(filename);
	title = g_strdup_printf("%s - %s", short_filename, __PROGRAM_NAME__);
	gtk_window_set_title(GTK_WINDOW(ui->window), title);

	ui->filename = filename;
	ui_save(ui);
	gtk_widget_set_sensitive(ui->btn_save, FALSE);

	g_free(short_filename);
	g_free(title);
    }

    gtk_widget_destroy(dialog);
}

static void ui_set_zoom_ctrls_enabled(UI *ui, gboolean setting)
{
    gtk_widget_set_sensitive(ui->btn_inczoom, setting);
    gtk_widget_set_sensitive(ui->btn_deczoom, setting);
    gtk_widget_set_sensitive(ui->btn_normzoom, setting);
    gtk_widget_set_sensitive(ui->btn_saveast, setting);
}

void ui_set_error(UI * ui, gchar * msg, ...)
{
    gchar		*buffer, *tmp;
    va_list		args;

    if (msg) {
	ui_set_zoom_ctrls_enabled(ui, FALSE);

	va_start(args, msg);
	buffer = g_strdup_vprintf(msg, args);
	va_end(args);

	gtk_image_set_from_stock(GTK_IMAGE(ui->frame_error_image),
                                 GTK_STOCK_DIALOG_ERROR,
                                 GTK_ICON_SIZE_MENU);

	tmp = g_strdup_printf("<small>%s</small>", buffer);
	gtk_label_set_markup(GTK_LABEL(ui->frame_error_label), tmp);

	g_free(buffer);
	g_free(tmp);
    } else {
	ui_set_zoom_ctrls_enabled(ui, TRUE);

	gtk_image_set_from_stock(GTK_IMAGE(ui->frame_error_image),
                                 GTK_STOCK_DIALOG_INFO,
                                 GTK_ICON_SIZE_MENU);

	gtk_label_set_markup(GTK_LABEL(ui->frame_error_label), "<small>Pronto.</small>");
    }
}

void ui_set_message(UI * ui, gchar * msg, ...)
{
    gchar		*buffer, *tmp;
    va_list		args;

    if (msg) {
	va_start(args, msg);
	buffer = g_strdup_vprintf(msg, args);
	va_end(args);

	gtk_image_set_from_stock(GTK_IMAGE(ui->frame_error_image),
                                 GTK_STOCK_DIALOG_INFO,
                                 GTK_ICON_SIZE_MENU);
                                 
	tmp = g_strdup_printf("<small>%s</small>", buffer);
	gtk_label_set_markup(GTK_LABEL(ui->frame_error_label), tmp);

	g_free(buffer);
	g_free(tmp);
    } else {
	gtk_image_set_from_stock(GTK_IMAGE(ui->frame_error_image),
                                 GTK_STOCK_DIALOG_INFO,
                                 GTK_ICON_SIZE_MENU);

	gtk_label_set_markup(GTK_LABEL(ui->frame_error_label), "<small>Pronto.</small>");
    }
}

/* adapted from gtksourceview's test-widget.c */
static void cb_update_cursor_pos(GtkTextBuffer *buffer,
                                 GtkTextIter   *iter,
                                 GtkTextMark   *mark,
                                 gpointer      user_data)
{
    GtkTextIter start;
    UI *ui = (UI *)user_data;
    gint tabwidth, chars, col, row;
    gchar *msg;
    
    if (mark != gtk_text_buffer_get_insert(buffer))
        return;

    tabwidth = gtk_source_view_get_tab_width(GTK_SOURCE_VIEW(ui->source_editor));

    chars = gtk_text_iter_get_offset(iter);
    row = gtk_text_iter_get_line(iter) + 1;

    start = *iter;
    gtk_text_iter_set_line_offset(&start, 0);
    col = 0;

    while (!gtk_text_iter_equal(&start, iter))
    {
            if (gtk_text_iter_get_char(&start) == '\t') {
                    col += (tabwidth - (col % tabwidth));
            } else {
                    col++;
            }

            gtk_text_iter_forward_char(&start);
    }

    msg = g_strdup_printf("Ln %d, Col %d", row, col + 1);
    gtk_label_set_text(GTK_LABEL(ui->lbl_lncol), msg);
    g_free(msg);
}

static void cb_update_toggle_ovr(GtkTextView *text_view,
                                 gpointer user_data)
{
    UI *ui = (UI *)user_data;
    gchar *msg;
    
    if (gtk_text_view_get_overwrite(GTK_TEXT_VIEW(ui->source_editor))) {
        msg = "INS";
    } else {
        msg = "OVR";
    }
    
    gtk_label_set_text(GTK_LABEL(ui->lbl_insovr), msg);
}

void ui_set_cursor(UI * ui, GdkCursorType cursor_type)
{
    GdkCursor		*cursor;

    cursor = gdk_cursor_new(cursor_type);
    gdk_window_set_cursor(GDK_WINDOW(ui->window->window), cursor);
    gdk_display_flush(gdk_display_get_default());
    gdk_cursor_unref(cursor);

    while (gtk_events_pending())
	gtk_main_iteration();
}

static gchar *ui_get_temp_file(gchar *extension)
{
    gchar 		*templ;
    int			 tmpfd;
    
    templ = g_strdup_printf("%s%scsdXXXXXX%d.%s",
                            g_get_tmp_dir(),
                            G_DIR_SEPARATOR_S,
                            getpid(),
                            extension);
                            
    if ((tmpfd = g_mkstemp(templ)) != -1) {
      close(tmpfd);
    }
    
    return templ;
}

static void ui_destroy_temp_file(gchar *temp_file)
{
    unlink(temp_file);
    g_free(temp_file);
}

void ui_show_performance(UI *ui, gchar *performance_output)
{
    GtkTreeIter	iter;
    gchar 	**steps, **step;
    gint	s;
    
    if (performance_output && conf_get_boolean("debug", "showtime", FALSE)) {
        gtk_list_store_clear(ui->store_performance);
        
        steps = g_strsplit(performance_output, "\n", 0);
        for (s = 0; steps[s]; s++) {
          step = g_strsplit(steps[s], "|", 0);
          
          if (step[0]) {
              gtk_list_store_append(ui->store_performance, &iter);
            
              gtk_list_store_set(ui->store_performance, &iter,
                                 P_PASS, g_strchug(step[0]),
                                 P_TIME, g_strchug(step[1]),
                                 P_PERCENTAGE, atof(step[2]),
                                 -1);
          }

          g_strfreev(step);
        }
        
        g_strfreev(steps);

        gtk_widget_show(ui->tv_performance);
    } else {
        gtk_widget_hide(ui->tv_performance);
    }
}

static gchar *ui_get_object_file(UI *ui)
{
    gchar *output_file, *temp;

    output_file = g_strdup(ui->filename);
    if ((temp = g_strrstr(output_file, "."))) {
      *temp = '\0';
    }
    temp = g_strdup_printf("%s.obj", output_file);
    g_free(output_file);
    
    return temp;
}

static void cb_btn_execute_clicked(GtkWidget * widget, gpointer data)
{
    UI		*ui = (UI *)data;
    gchar 	*mvd_location, *cmdline, *objfile;
    GError	*error = NULL;
    
    if (conf_get_integer("compiler", "outputlanguage", 0) != 0) {
	GtkWidget	*dialog;

	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Linguagem de saída incompatível com a Máquina Virtual Didática.");
        gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
        return;
    }
    
    mvd_location = conf_get_string("run", "virtualmachine", "");
    if (!g_file_test(mvd_location, G_FILE_TEST_IS_EXECUTABLE)) {
	GtkWidget	*dialog;

	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Máquina virtual não selecionada ou não executável.");
        gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
        g_free(mvd_location);
        
        gtk_widget_show(ui->options_window);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(ui->option_notebook), 3);
        gtk_widget_grab_focus(ui->option_btn_mvd_location);
        
        return;
    }
    
    objfile = ui_get_object_file(ui);
    cmdline = g_strdup_printf("'%s' '%s'", mvd_location, objfile);

    if (!g_spawn_command_line_async(cmdline, &error)) {
	GtkWidget	*dialog;
	
	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Impossível executar a máquina virtual (%s).",
					error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
        g_error_free(error);
    }
    
    g_free(cmdline);
    g_free(mvd_location);
    g_free(objfile);
}

static void cb_btn_compile_clicked(GtkWidget * widget, gpointer data)
{
    gchar	*std_output, *std_error, *command_line, *source_code;
    gchar	*temp_output;
    gint	optimization_level = 0, exit_status;
    GError	*error = NULL;
    UI		*ui = (UI *) data;
    GtkTextIter	start_iter, end_iter;
    gboolean	viagem = conf_get_boolean("compiler", "viagem-do-freitas", TRUE);
    
    ui_set_cursor(ui, GDK_WATCH);
    ui_set_error(ui, NULL);
    ui_show_performance(ui, NULL);
    
    optimization_level += gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				     (ui->option_chk_optl1)) ? 1 : 0;
    optimization_level += gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				     (ui->option_chk_optl2)) ? 2 : 0;

    temp_output = ui_get_temp_file("csd");

    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(ui->source_buffer), &start_iter);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(ui->source_buffer), &end_iter);
    source_code = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(ui->source_buffer),
				           &start_iter, &end_iter, TRUE);
    g_file_set_contents(temp_output, source_code, -1, NULL);
    g_free(source_code);

    command_line = g_strdup_printf("'%s' '%s' -t -O %d %s",
                                   argv0,				/* ourself */
                                   temp_output,				/* input file */
				   optimization_level,			/* optimization level */
				   viagem ? "-v" : "");			/* viagem do Freitas */
				   
    g_spawn_command_line_sync(command_line, &std_output, &std_error,
			      &exit_status, &error);

    if (error) {
        ui_set_error(ui, "Erro ao invocar o compilador: %s", error->message);
	g_error_free(error);
    } else if (!WIFEXITED(exit_status)) {
        if (WIFSIGNALED(exit_status)) {
            ui_set_error(ui, "Compilador abortou com sinal %d (<b>%s</b>).",
                         WTERMSIG(exit_status), g_strsignal(WTERMSIG(exit_status)));
        } else {
            ui_set_error(ui, "Compilador abortou de forma inesperada.");
        }
    } else if (WEXITSTATUS(exit_status) == 0) {
        gchar *output_file;
        
        output_file = ui_get_object_file(ui);
        
        g_file_set_contents(output_file, std_output, -1, NULL);
        gtk_widget_set_sensitive(ui->btn_compile, FALSE);
        gtk_widget_set_sensitive(ui->btn_execute, TRUE);
        gtk_widget_set_sensitive(ui->btn_ast, TRUE);
        
        ui_set_message(ui, "Compilado com sucesso.");
        ui_show_performance(ui, std_error);
        
        g_free(output_file);
    } else if (WEXITSTATUS(exit_status) == 1) {
	std_output = g_strstrip(std_output);

	ui_set_error(ui, std_output);
	ui_scroll_to_error(ui, std_output);
    } else {
        ui_set_error(ui, "Estado de saída \"%d\" desconhecido retornado pelo compilador.", exit_status);
    }

    g_free(std_error);
    g_free(std_output);
    g_free(command_line);
    
    ui_destroy_temp_file(temp_output);

    ui_set_cursor(ui, GDK_LEFT_PTR);
}

static void cb_btn_about_clicked(GtkWidget * widget, gpointer data)
{
    GtkWidget *about;
    GdkPixbuf *logo;
    const gchar *artists[] = {
	"Mattahan (Paul Davey)",
	NULL
    };

    logo = gdk_pixbuf_new_from_inline(-1, hammer, FALSE, NULL);

    about = gtk_about_dialog_new();
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), __PROGRAM_NAME__);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),
				 __PROGRAM_VERSION__);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
				   "Copyright \302\251 2007-2008 "
				   "Leandro A. F. Pereira");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
				  "Compilador para a Linguagem Simplificada Didática");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), logo);
    gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);

    gdk_pixbuf_unref(logo);
}

static void cb_btn_options_clicked(GtkWidget * widget, gpointer data)
{
    UI *ui = (UI *) data;

    gtk_widget_show(ui->options_window);
    gtk_window_present(GTK_WINDOW(ui->options_window));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ui->option_notebook), 0);
}

static void cb_btn_ast_clicked(GtkWidget * widget, gpointer data)
{
    gchar	*std_output, *std_error, *command_line, *source_code;
    gchar	*temp_output, *temp_image;
    gint 	exit_status;
    GError 	*error = NULL;
    UI 		*ui = (UI *) data;
    GtkTextIter start_iter, end_iter;
    gboolean	viagem = conf_get_boolean("compiler", "viagem-do-freitas", TRUE);

    if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(ui->btn_ast))) {
      /* volta para o editor */
      gtk_notebook_set_page(GTK_NOTEBOOK(ui->notebook), 0);
      return;
    }

    ui_set_error(ui, NULL);
    ui_set_cursor(ui, GDK_WATCH);
    gtk_notebook_set_page(GTK_NOTEBOOK(ui->notebook), 1);

    temp_image = ui_get_temp_file("svg");
    temp_output = ui_get_temp_file("dot");

    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(ui->source_buffer), &start_iter);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(ui->source_buffer), &end_iter);
    source_code = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(ui->source_buffer),
		                           &start_iter, &end_iter, TRUE);
    g_file_set_contents(temp_output, source_code, -1, NULL);
    g_free(source_code);

    command_line = g_strdup_printf("'%s' -A '%s' %s", argv0, temp_output,
                                   viagem ? "-v" : "");
    g_spawn_command_line_sync(command_line, &std_output, &std_error,
			      &exit_status, &error);

    exit_status = WEXITSTATUS(exit_status);

    if (error) {
	g_error_free(error);
    } else if (exit_status == 0) {
	g_file_set_contents(temp_output, std_output, -1, NULL);
	g_free(std_output);
	g_free(command_line);
	
	command_line = g_strdup_printf("dot -Tsvg -o '%s' '%s'", temp_image, temp_output);

	if (g_spawn_command_line_sync(command_line, &std_output, NULL, &exit_status, NULL)) {
	    ui->ast_zoom_factor = 1.0f;
	    ui->ast_pixbuf = gdk_pixbuf_new_from_file(temp_image, NULL);
	    gtk_image_set_from_pixbuf(GTK_IMAGE(ui->ast_img), ui->ast_pixbuf);
            goto cleanup;
	} else {
	    ui_set_error(ui, "\"dot\" não encontrado. Instale o Graphviz para visualizar a AST.");
	}
    } else if (exit_status == 1) {
	std_output = g_strstrip(std_output);

	ui_set_error(ui, std_output);
	ui_scroll_to_error(ui, std_output);
    } else {
        ui_set_error(ui, "\"dot\" retornou um status desconhecido (%d).", exit_status);
    }
    
    gtk_image_set_from_stock(GTK_IMAGE(ui->ast_img), GTK_STOCK_DIALOG_ERROR,
                             GTK_ICON_SIZE_DIALOG);

cleanup:
    ui_set_cursor(ui, GDK_LEFT_PTR);

    g_free(std_error);
    g_free(std_output);
    g_free(command_line);
    
    ui_destroy_temp_file(temp_output);
    ui_destroy_temp_file(temp_image);
}

static void cb_source_buffer_changed(GtkTextBuffer *buffer,
                                     gpointer user_data)
{
    UI *ui = (UI *)user_data;
    
    gtk_widget_set_sensitive(ui->btn_save, TRUE);
    ui_show_performance(ui, NULL);
    
    if (ui->filename) {
      gtk_widget_set_sensitive(ui->btn_compile, TRUE);
      gtk_widget_set_sensitive(ui->btn_ast, TRUE);
      gtk_widget_set_sensitive(ui->btn_execute, FALSE);
    }
}

static void cb_btn_hide_options_clicked(GtkWidget *widget,
                                        gpointer user_data) 
{
    UI *ui = (UI *)user_data;
    
    gtk_widget_hide(ui->options_window);
}

static void ui_ast_zoom(UI *ui, gfloat factor)
{
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    gint width, height;
    
    if (!ui->ast_pixbuf) {
        return;
    }

    ui->ast_zoom_factor += factor;

    if (factor == 0.0f || ui->ast_zoom_factor == 1.0f) {
      ui->ast_zoom_factor = 1.0f;
      gtk_image_set_from_pixbuf(GTK_IMAGE(ui->ast_img), ui->ast_pixbuf);

      return;
    }

    pixbuf = ui->ast_pixbuf;
    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);
    
    width = (gint)((float)width * ui->ast_zoom_factor);
    height = (gint)((float)height * ui->ast_zoom_factor);
    
    scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_HYPER);
    gtk_image_set_from_pixbuf(GTK_IMAGE(ui->ast_img), scaled_pixbuf);
    
    g_object_unref(scaled_pixbuf);
}

static void cb_btn_inczoom_clicked(GtkWidget *widget,
                                   gpointer user_data)
{
    UI *ui = (UI *)user_data;
    
    ui_ast_zoom(ui, +0.1f);
    
    gtk_widget_set_sensitive(ui->btn_deczoom, TRUE);
    if (ui->ast_zoom_factor > 2.0f) {
      gtk_widget_set_sensitive(ui->btn_inczoom, FALSE);
    }
}

static void cb_btn_deczoom_clicked(GtkWidget *widget,
                                   gpointer user_data)
{
    UI *ui = (UI *)user_data;
    
    ui_ast_zoom(ui, -0.1f);

    gtk_widget_set_sensitive(ui->btn_inczoom, TRUE);
    if (ui->ast_zoom_factor <= 0.1f) {
      gtk_widget_set_sensitive(ui->btn_deczoom, FALSE);
    }
}

static void cb_btn_normzoom_clicked(GtkWidget *widget,
                                    gpointer user_data)
{
    UI *ui = (UI *)user_data;

    ui_ast_zoom(ui, 0.0f);
    
    gtk_widget_set_sensitive(ui->btn_inczoom, TRUE);
    gtk_widget_set_sensitive(ui->btn_deczoom, TRUE);
}

static void cb_btn_saveast_clicked(GtkWidget *widget,
                                   gpointer user_data)
{
    UI 			*ui = (UI *)user_data;
    GtkFileFilter	*filter;
    GtkWidget		*dialog;

    dialog = gtk_file_chooser_dialog_new("Salvar AST",
					 GTK_WINDOW(ui->window),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE,
					 GTK_RESPONSE_ACCEPT, NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.[Pp][Nn][Gg]");
    gtk_file_filter_set_name(filter, "Arquivos de Imagem (*.png)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        gdk_pixbuf_save(ui->ast_pixbuf, filename, "png", NULL, NULL);

	g_free(filename);
    }
    
    gtk_widget_destroy(dialog);    
}

static GtkSourceLanguageManager *ui_get_default_language_manager()
{
    static GtkSourceLanguageManager	*lm = NULL;
    gchar				**lang_dirs, *temp;
    const gchar 			*const *default_lang_dirs;
    gint				nlang_dirs, i;

    temp = g_build_filename(g_get_home_dir(), ".csd", "lpd.lang", NULL);
    if (!g_file_test(temp, G_FILE_TEST_EXISTS)) {
      g_file_set_contents(temp, lpd_lang, sizeof(lpd_lang), NULL);
    }
    g_free(temp);

    if (lm == NULL) {
	lm = gtk_source_language_manager_new();

	nlang_dirs = 0;
	default_lang_dirs =
	    gtk_source_language_manager_get_search_path
	    (gtk_source_language_manager_get_default());
	for (nlang_dirs = 0; default_lang_dirs[nlang_dirs] != NULL;
	     nlang_dirs++);

	lang_dirs = g_new0(gchar *, nlang_dirs + 2);
	for (i = 0; i < nlang_dirs; i++) {
	    lang_dirs[i] = (gchar *) default_lang_dirs[i];
	}
	lang_dirs[nlang_dirs] = g_build_filename(g_get_home_dir(), ".csd", NULL);

	gtk_source_language_manager_set_search_path(lm, lang_dirs);
    }

    return lm;
}

static void cb_update_tabsize(gpointer user_data, gpointer new_value)
{
    UI *ui = (UI *)user_data;
    
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(ui->source_editor),
                                  GPOINTER_TO_INT(new_value));
}

static void cb_update_autoindent(gpointer user_data, gpointer new_value)
{
    UI *ui = (UI *)user_data;
    
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(ui->source_editor),
                                    GPOINTER_TO_INT(new_value));
}

static void cb_update_highlightcurrline(gpointer user_data, gpointer new_value)
{
    UI *ui = (UI *)user_data;

    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(ui->source_editor),
                                               GPOINTER_TO_INT(new_value));
}

static void cb_update_showlines(gpointer user_data, gpointer new_value)
{
    UI *ui = (UI *)user_data;

    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(ui->source_editor),
                                          GPOINTER_TO_INT(new_value));
}

static void cb_update_stylescheme(gpointer user_data, gpointer new_value)
{
    GtkSourceStyleScheme *scheme;
    GtkSourceStyleSchemeManager *sm;
    UI *ui = (UI *)user_data;
    gchar *scheme_name;
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui->option_cmb_style), GPOINTER_TO_INT(new_value));
    scheme_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui->option_cmb_style));
    
    if (scheme_name) {
        sm = gtk_source_style_scheme_manager_get_default();
        scheme = gtk_source_style_scheme_manager_get_scheme(sm, scheme_name);
        if (scheme) {
            gtk_source_buffer_set_style_scheme(ui->source_buffer, scheme);
        }
    }
}

static void cb_update_font(gpointer user_data, gpointer new_value)
{
    PangoFontDescription	*font_desc;
    UI				*ui = (UI *)user_data;

    font_desc = pango_font_description_from_string(new_value);
    gtk_widget_modify_font(ui->source_editor, font_desc);     
    pango_font_description_free(font_desc);
}

static void populate_stylescheme(UI *ui)
{
    GtkSourceStyleSchemeManager *sm;
    const gchar * const * schemes;
    
    sm = gtk_source_style_scheme_manager_get_default();
    
    for (schemes = gtk_source_style_scheme_manager_get_scheme_ids(sm); *schemes; schemes++) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(ui->option_cmb_style), *schemes);
    }
}

#define WIDGET(structname,xmlname)	ui->structname = glade_xml_get_widget(ui->gxml, xmlname)
#define SIGNAL(widget,signal,handler)	g_signal_connect(G_OBJECT(ui->widget), signal, G_CALLBACK(handler), ui)

UI *ui_new()
{
    GtkSourceLanguage		*language;
    GtkSourceLanguageManager	*manager;
    UI				*ui;
    GdkPixbuf			*icon;
    TreeViewColumn 		performance_columns[] = {
      { "Passo",		RENDERER_TEXT,		P_PASS },
      { "Tempo",		RENDERER_TEXT,		P_TIME },
      { "Percentual",		RENDERER_PERCENTAGE,	P_PERCENTAGE },
    };

    ui = g_new0(UI, 1);

    ui->gxml = glade_xml_new_from_buffer(compiler_glade,
					 sizeof(compiler_glade),
					 NULL, NULL);
    if (!ui->gxml) {
	g_error("Cannot parse Glade XML");
    }

    WIDGET(window, "compiler");
    WIDGET(options_window, "options");
    WIDGET(notebook, "notebook");
    WIDGET(scroll_editor, "scroll_editor_area");

    WIDGET(frame_error_label, "frame_error_label");
    WIDGET(frame_error_image, "frame_error_image");

    WIDGET(btn_new, "btn_new");
    WIDGET(btn_open, "btn_open");
    WIDGET(btn_save, "btn_save");
    WIDGET(btn_compile, "btn_compile");
    WIDGET(btn_execute, "btn_execute");
    WIDGET(btn_options, "btn_options");
    WIDGET(btn_ast, "btn_ast");
    WIDGET(btn_about, "btn_about");
    
    WIDGET(btn_inczoom, "btn_inczoom");
    WIDGET(btn_deczoom, "btn_deczoom");
    WIDGET(btn_normzoom, "btn_normzoom");
    WIDGET(btn_saveast, "btn_saveast");

    WIDGET(option_cmb_output_language, "cmb_output_language");
    WIDGET(option_cmb_style, "cmb_style");
    WIDGET(option_chk_optl1, "chk_opt_level1");
    WIDGET(option_chk_optl2, "chk_opt_level2");
    WIDGET(option_chk_showtime, "chk_opt_showtime");
    WIDGET(option_chk_dupl_avo, "chk_opt_dupl_avo");
    WIDGET(option_spn_tabsize, "spn_tabsize");
    WIDGET(option_chk_autoindent, "chk_opt_autoindent");
    WIDGET(option_chk_highlightcurrline, "chk_opt_highlightcurrline");
    WIDGET(option_chk_showlines, "chk_opt_showlines");
    WIDGET(option_btn_fontchooser, "btn_fontchooser");
    WIDGET(option_btn_mvd_location, "btn_mvd_location");
    WIDGET(option_notebook, "opt_notebook");

    WIDGET(ast_img, "ast_img");
    WIDGET(tv_performance, "tv_performance");
    WIDGET(lbl_lncol, "lbl_lncol");
    WIDGET(lbl_insovr, "lbl_insovr");
    
    WIDGET(btn_hide_options, "btn_hide_options");

    SIGNAL(window, "destroy", cb_window_destroy);
    SIGNAL(btn_new, "clicked", cb_btn_new_clicked);
    SIGNAL(btn_open, "clicked", cb_btn_open_clicked);
    SIGNAL(btn_save, "clicked", cb_btn_save_clicked);
    SIGNAL(btn_compile, "clicked", cb_btn_compile_clicked);
    SIGNAL(btn_execute, "clicked", cb_btn_execute_clicked);
    SIGNAL(btn_about, "clicked", cb_btn_about_clicked);
    SIGNAL(btn_hide_options, "clicked", cb_btn_hide_options_clicked);

    SIGNAL(btn_options, "clicked", cb_btn_options_clicked);
    SIGNAL(btn_ast, "clicked", cb_btn_ast_clicked);
    
    SIGNAL(btn_inczoom, "clicked", cb_btn_inczoom_clicked);
    SIGNAL(btn_deczoom, "clicked", cb_btn_deczoom_clicked);
    SIGNAL(btn_normzoom, "clicked", cb_btn_normzoom_clicked);
    SIGNAL(btn_saveast, "clicked", cb_btn_saveast_clicked);
    
    ui->source_buffer = gtk_source_buffer_new(NULL);
    ui->source_editor = gtk_source_view_new_with_buffer(ui->source_buffer);
    GTK_WIDGET_SET_FLAGS(ui->source_editor, GTK_CAN_FOCUS);

    SIGNAL(source_buffer, "changed", cb_source_buffer_changed);
    SIGNAL(source_buffer, "mark-set", cb_update_cursor_pos);
    SIGNAL(source_editor, "toggle-overwrite", cb_update_toggle_ovr);
    
    populate_stylescheme(ui);

    conf_link_combobox(GTK_COMBO_BOX(ui->option_cmb_output_language),
                       "compiler", "outputlanguage", 0, NULL, NULL);
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_optl1),
                          "optimization", "level1", FALSE, NULL, NULL);
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_optl2),
                          "optimization", "level2", FALSE, NULL, NULL);
    
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_showtime),
                          "debug", "showtime", FALSE, NULL, NULL);
    
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_dupl_avo),
                          "compiler", "viagem-do-freitas", TRUE, NULL, NULL);
    
    conf_link_spinbutton(GTK_SPIN_BUTTON(ui->option_spn_tabsize),
                         "tab", "width", 8, cb_update_tabsize, ui);
    
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_autoindent),
                          "editor", "autoindent", TRUE, cb_update_autoindent, ui);
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_highlightcurrline),
                          "editor", "highlightcurrline", TRUE, cb_update_highlightcurrline, ui);
    conf_link_checkbutton(GTK_CHECK_BUTTON(ui->option_chk_showlines),
                          "editor", "showlines", TRUE, cb_update_showlines, ui);
    conf_link_combobox(GTK_COMBO_BOX(ui->option_cmb_style),
                       "editor", "stylescheme", 0, cb_update_stylescheme, ui);
    
    conf_link_fontbutton(GTK_FONT_BUTTON(ui->option_btn_fontchooser),
                         "editor", "font", "monospace 10", cb_update_font, ui);
    conf_link_filebutton(GTK_FILE_CHOOSER_BUTTON(ui->option_btn_mvd_location),
                         "run", "virtualmachine", "", NULL, NULL);

    gtk_source_view_set_indent_on_tab(GTK_SOURCE_VIEW(ui->source_editor), TRUE);

    manager = ui_get_default_language_manager();
    language = gtk_source_language_manager_get_language(manager, "lpd");
    gtk_source_buffer_set_language(ui->source_buffer, language);

    gtk_container_add(GTK_CONTAINER(ui->scroll_editor), ui->source_editor);
    gtk_widget_show(ui->source_editor);

    ui->store_performance = gtk_list_store_new(N_PERFORMANCE_COLUMNS,
                                               G_TYPE_STRING,	/* pass */
                                               G_TYPE_STRING,	/* time */
                                               G_TYPE_FLOAT);	/* percentage */
    gtk_tree_view_set_model(GTK_TREE_VIEW(ui->tv_performance),
                            GTK_TREE_MODEL(ui->store_performance));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui->tv_performance), TRUE);
    
    tree_view_add_columns(GTK_TREE_VIEW(ui->tv_performance), performance_columns,
                          G_N_ELEMENTS(performance_columns));

    icon = gdk_pixbuf_new_from_inline(-1, hammer, FALSE, NULL);
    gtk_window_set_icon(GTK_WINDOW(ui->window), icon);
    gdk_pixbuf_unref(icon);

    ui_reset(ui);
    ui_set_message(ui, NULL);
    gtk_widget_show(ui->window);
    gtk_widget_hide(ui->options_window);

    return ui;
}

void ui_destroy(UI * ui)
{
    g_object_unref(G_OBJECT(ui->gxml));
    g_free(ui);
}
