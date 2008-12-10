/*
 * Simple Pascal Compiler
 * Virtual Machine (User Interface)
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include "ui.h"
#include "vm.h"
#include "treeview.h"

#include "arrow.h"
#include "gear.h"
#include "mvd_glade.h"

static void
log_handler(const gchar * log_domain,
            GLogLevelFlags log_level,
            const gchar * message, gpointer user_data)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL,
                                                (log_level &
                                                 G_LOG_FLAG_FATAL) ?
                                                GTK_MESSAGE_ERROR :
                                                GTK_MESSAGE_WARNING,
                                                GTK_BUTTONS_CLOSE,
                                                "<big><b>%s</b></big>\n\n%s",
                                                (log_level &
                                                 G_LOG_FLAG_FATAL) ?
                                                "Erro Fatal" :
                                                "Aviso", message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void
ui_console_clear(UI *ui)
{
  GtkTextBuffer	*buffer;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->output));
  gtk_text_buffer_set_text(buffer, "", -1);
}

static void
ui_console_write(UI *ui, const char *format, ...)
{
  GtkTextBuffer *buffer;
  GtkTextIter	 iter;
  va_list	 args;
  gchar		*message;

  va_start(args, format);
  message = g_strdup_vprintf(format, args);
  va_end(args);
  
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->output));
  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_buffer_insert(buffer, &iter, message, -1);

  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ui->output), &iter,
                               0.1f, FALSE, 0.0f, 0.0f);
  gtk_text_buffer_place_cursor(buffer, &iter);
  g_free(message);
}

static void
ui_update_ui(UI *ui)
{
  VMInstruction	*instruction;
  GtkTreePath *path;
  GtkTreeIter *iter;   
  GtkTreeSelection *selection;
  int i;
  
  if (!ui->vm->instruction_pointer) {
    gtk_widget_set_sensitive(ui->btn_step_by_step, FALSE);
    ui_console_write(ui, "\342\212\227\tExecução terminada.\n\n");
    return;
  }
  
  /* atualiza o ponteiro da lista de instrucoes */
  ui->selection_changeable = TRUE;

  instruction = (VMInstruction *)ui->vm->instruction_pointer->data;
  iter = (GtkTreeIter *)instruction->data;
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(ui->store_instructions), iter);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui->tv_instructions));

  gtk_tree_selection_select_iter(selection, iter);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(ui->tv_instructions), path, NULL, FALSE);
  gtk_tree_path_free(path);
  
  gtk_list_store_set(ui->store_instructions, iter,
                     IC_POINTER, ui->pbuf_arrow, -1);

  ui->selection_changeable = FALSE;

  /* atualiza memoria */
  gtk_list_store_clear(ui->store_memory);
  for (i = 0; i <= ui->vm->stack_top; i++) {
    GtkTreeIter miter;
    
    gtk_list_store_append(ui->store_memory, &miter);
    gtk_list_store_set(ui->store_memory, &miter,
                       MC_ADDRESS, i,
                       MC_CONTENT, ui->vm->memory[i],
                       -1);
  }
}

static void
ui_display_object(UI *ui)
{
  VMInstruction *instruction;
  GtkTreeIter iter;
  GList *line;
  gchar *label;
  guint line_no = 1;

  gtk_list_store_clear(ui->store_instructions);
  
  for (line = ui->vm->program; line; line = line->next, line_no++) {
    instruction = (VMInstruction *)line->data;
    
    gtk_list_store_append(ui->store_instructions, &iter);

    if (instruction->opcode == OP_LABEL) {
      label = g_strdup_printf("<b><span color=\"darkslategray\">%s:</span></b>",
                              instruction->label_name);
    } else {
      label = g_strdup_printf("<b><span color=\"#ccc\">%d</span></b>",
                              line_no);
    }

    gtk_list_store_set(ui->store_instructions, &iter, IC_LABEL, label, -1);
    g_free(label);

    gtk_list_store_set(ui->store_instructions, &iter,
                       IC_NAME, instructions[instruction->opcode].name,
                       IC_PARAM1, instruction->sparam1,
                       IC_PARAM2, instruction->sparam2,
                       -1);
    
    instruction->data = gtk_tree_iter_copy(&iter);
  }
}

void
ui_step(UI *ui)
{
  VMInstruction	*instruction;
  
  instruction = (VMInstruction *)ui->vm->instruction_pointer->data;

  ui->vm->running = TRUE;
  vm_step(ui->vm);

  gtk_list_store_set(ui->store_instructions, (GtkTreeIter *)instruction->data,
                     IC_POINTER, NULL, -1);

  ui_update_ui(ui);
  
  if (!ui->vm->running && !ui->vm->instruction_pointer) {
    gtk_widget_set_sensitive(ui->btn_step_by_step, FALSE);
  }
  
  while (gtk_events_pending())
    gtk_main_iteration();  
}

void
ui_stop(UI *ui)
{
  ui->vm->running = FALSE;
}


void
ui_reset(UI *ui, gboolean clear_output)
{
  VMInstruction	*instruction;
  GList		*list;
  
  if (GTK_WIDGET_VISIBLE(ui->input)) {
    gtk_entry_set_text(GTK_ENTRY(ui->input_box), "");
    gtk_widget_hide(ui->input);
    gtk_main_quit();
  }

  for (list = ui->vm->program; list; list = list->next) {
    instruction = (VMInstruction *)list->data;
    gtk_list_store_set(ui->store_instructions, (GtkTreeIter *)instruction->data,
                       IC_POINTER, NULL, -1);
  }

  if (clear_output) {
    ui_console_clear(ui);
  }

  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ui->btn_execute), FALSE);
  gtk_widget_set_sensitive(ui->btn_step_by_step, TRUE);

  vm_reset(ui->vm);
  ui_update_ui(ui);
}

void
ui_execute(UI *ui)
{
  ui->vm->running = TRUE;
  
  while (ui->vm->running) {
    vm_step(ui->vm);
    
    while (gtk_events_pending())
      gtk_main_iteration();
  }
  
  ui_reset(ui, FALSE);
  ui_console_write(ui, "\342\212\227\tExecução terminada.\n\n");
}


static void cb_window_destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

static void ui_object_load(UI *ui, char *object_file)
{
    char *short_filename;
    char *title;

    if (!object_file)
        return;

    vm_object_load(ui->vm, (const char *)object_file);
    ui_console_clear(ui);
    ui_display_object(ui);
    ui_update_ui(ui);

    gtk_widget_set_sensitive(ui->btn_execute, TRUE);
    gtk_widget_set_sensitive(ui->btn_reset, TRUE);
    gtk_widget_set_sensitive(ui->btn_step_by_step, TRUE);

    short_filename = g_path_get_basename(object_file);
    title = g_strdup_printf("%s - Máquina Virtual Didática", short_filename);
    gtk_window_set_title(GTK_WINDOW(ui->window), title);

    g_free(short_filename);
    g_free(title);
}

static void cb_btn_open_clicked(GtkWidget *widget, gpointer data)
{
  UI 		*ui = (UI *)data;
  GtkFileFilter	*filter;
  GtkWidget 	*dialog;

  dialog = gtk_file_chooser_dialog_new("Abrir Objeto",
                                       GTK_WINDOW(ui->window),
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                       NULL);

  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*.[Oo][Bb][Jj]");
  gtk_file_filter_set_name(filter, "Arquivos Objeto (*.obj)");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      gchar *filename;

      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

      ui_object_load(ui, filename);

      g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

static void cb_btn_execute_clicked(GtkWidget *widget, gpointer data)
{
  UI *ui = (UI *)data;
  
  if (GTK_WIDGET_VISIBLE(ui->input)) {
    gtk_entry_set_text(GTK_ENTRY(ui->input_box), "");
    gtk_widget_hide(ui->input);
    gtk_main_quit();
    
    ui_console_clear(ui);
  }

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget))) {
    if (!GTK_WIDGET_SENSITIVE(ui->btn_step_by_step)) {
      ui_reset(ui, TRUE);
    }
    
    gtk_widget_set_sensitive(ui->btn_open, FALSE);
    gtk_widget_set_sensitive(ui->btn_step_by_step, FALSE);

    ui_execute(ui);
  } else {
    gtk_widget_set_sensitive(ui->btn_open, TRUE);
    gtk_widget_set_sensitive(ui->btn_step_by_step, TRUE);
  
    ui_stop(ui);
  }
}

static void cb_btn_step_by_step_clicked(GtkWidget *widget, gpointer data)
{
  UI *ui = (UI *)data;
  
  ui_step(ui);
}

static void cb_btn_reset_clicked(GtkWidget *widget, gpointer data)
{
  UI *ui = (UI *)data;
  
  ui_reset(ui, TRUE);
}

static void cb_btn_about_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *about;
    GdkPixbuf *logo;
    const gchar *artists[] = {
	"Mattahan (Paul Davey)",
	NULL
    };

    logo = gdk_pixbuf_new_from_inline(-1, gear, FALSE, NULL);

    about = gtk_about_dialog_new();
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "Máquina Virtual Didática");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "1.0");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
				   "Copyright \302\251 2008 "
				   "Leandro A. F. Pereira");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
				  "Executa código gerado pelo Compilador Didático");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), logo);
    gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);
    gdk_pixbuf_unref(logo);
}


void
ui_write(gpointer data, char *string)
{
  UI *ui = (UI *)data;
  
  ui_console_write(ui, "\342\206\222\t%s\n", string);
}

char *
ui_read(gpointer data)
{
  UI *ui = (UI *)data;
  static gchar *text = NULL;
  
  if (text) {
    g_free(text);
  }
  
  gtk_widget_set_sensitive(ui->btn_open, FALSE);
  gtk_widget_set_sensitive(ui->btn_step_by_step, FALSE);

  gtk_widget_show(ui->input);
  gtk_widget_grab_focus(ui->input_box);
  gtk_window_set_default(GTK_WINDOW(ui->window), ui->input_enter);
  
  gtk_main();
  
  text = g_strdup((gchar *)gtk_entry_get_text(GTK_ENTRY(ui->input_box)));
  gtk_entry_set_text(GTK_ENTRY(ui->input_box), "");
  
  gtk_widget_hide(ui->input);
  gtk_window_set_default(GTK_WINDOW(ui->window), NULL);

  gtk_widget_set_sensitive(ui->btn_open, TRUE);
  gtk_widget_set_sensitive(ui->btn_step_by_step, TRUE);
  
  ui_console_write(ui, "\342\206\220\t%s\n", text);
  
  return text;
}

void
cb_input_enter(GtkWidget *widget, gpointer data)
{
  UI *ui = (UI *)data;
  
  if (GTK_WIDGET_VISIBLE(ui->input)) {
    gtk_main_quit();
  }
}

static gboolean
cb_view_selection_func (GtkTreeSelection *selection,
                        GtkTreeModel     *model,
                        GtkTreePath      *path,
                        gboolean          path_currently_selected,
                        gpointer          userdata)
{
  UI *ui = (UI *) userdata;
  
  return ui->selection_changeable;
}

#define WIDGET(structname,xmlname)	ui->structname = glade_xml_get_widget(ui->gxml, xmlname)
#define SIGNAL(widget,signal,handler)	g_signal_connect(G_OBJECT(ui->widget), signal, G_CALLBACK(handler), ui)

UI *
ui_new(char *object_file)
{
  UI			*ui;
  GdkPixbuf             *icon;
  GtkTreeSelection	*selection;
  TreeViewColumn 	instruction_columns[] = {
    { "",		RENDERER_PIXBUF,	IC_POINTER },
    { "Rótulo",		RENDERER_TEXT,		IC_LABEL },
    { "Instrução",	RENDERER_TEXT,		IC_NAME },
    { "Arg. 1",		RENDERER_TEXT,		IC_PARAM1 },
    { "Arg. 2",		RENDERER_TEXT,		IC_PARAM2 }
  };
  TreeViewColumn	memory_columns[] = {
    { "Endereço",	RENDERER_TEXT,		MC_ADDRESS },
    { "Conteúdo",	RENDERER_TEXT,		MC_CONTENT }
  };
  
  ui = g_new0(UI, 1);

  ui->vm   = vm_new((VMReadFunction)  ui_read, ui,
                    (VMWriteFunction) ui_write, ui);

  ui->gxml = glade_xml_new_from_buffer(mvd_glade,
                                       sizeof(mvd_glade),
                                       NULL, NULL);
  if (!ui->gxml) {
    g_error("Cannot parse Glade XML");
  }
  
  ui->pbuf_arrow = gdk_pixbuf_new_from_inline(-1, arrow, FALSE, NULL);
  
  WIDGET(window, "mvd");
  WIDGET(notebook, "notebook");
  WIDGET(tv_instructions, "instructions");
  WIDGET(tv_memory, "memory");
  WIDGET(output, "output");
  WIDGET(input_box, "input_box");
  WIDGET(input, "input");
  WIDGET(input_enter, "input_enter");
  WIDGET(btn_open, "btn_open");
  WIDGET(btn_execute, "btn_execute");
  WIDGET(btn_step_by_step, "btn_step_by_step");
  WIDGET(btn_reset, "btn_reset");
  WIDGET(btn_about, "btn_about");
  
  SIGNAL(window, "destroy", cb_window_destroy);
  SIGNAL(input_enter, "clicked", cb_input_enter);
  SIGNAL(btn_open, "clicked", cb_btn_open_clicked);
  SIGNAL(btn_execute, "clicked", cb_btn_execute_clicked);
  SIGNAL(btn_step_by_step, "clicked", cb_btn_step_by_step_clicked);
  SIGNAL(btn_reset, "clicked", cb_btn_reset_clicked);
  SIGNAL(btn_about, "clicked", cb_btn_about_clicked);
  
  gtk_widget_set_sensitive(ui->btn_execute, FALSE);
  gtk_widget_set_sensitive(ui->btn_reset, FALSE);
  gtk_widget_set_sensitive(ui->btn_step_by_step, FALSE);

  gtk_entry_set_activates_default(GTK_ENTRY(ui->input_box), TRUE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(ui->output), FALSE);
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui->tv_instructions));
  gtk_tree_selection_set_select_function(selection, cb_view_selection_func, ui, NULL);

  ui->store_instructions = gtk_list_store_new(N_INSTRUCTION_COLUMNS,
                                              GDK_TYPE_PIXBUF,	/* pointer */
                                              G_TYPE_STRING,	/* label */
                                              G_TYPE_STRING,	/* name */
                                              G_TYPE_STRING,	/* param1 */
                                              G_TYPE_STRING);	/* param2 */
  gtk_tree_view_set_model(GTK_TREE_VIEW(ui->tv_instructions),
                          GTK_TREE_MODEL(ui->store_instructions));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui->tv_instructions), TRUE);
  
  tree_view_add_columns(GTK_TREE_VIEW(ui->tv_instructions), instruction_columns,
                        G_N_ELEMENTS(instruction_columns));
  
  ui->store_memory = gtk_list_store_new(N_MEMORY_COLUMNS,
                                        G_TYPE_INT,	/* address */
                                        G_TYPE_INT);	/* content */
  gtk_tree_view_set_model(GTK_TREE_VIEW(ui->tv_memory),
                          GTK_TREE_MODEL(ui->store_memory));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui->tv_instructions), TRUE);

  tree_view_add_columns(GTK_TREE_VIEW(ui->tv_memory), memory_columns,
                        G_N_ELEMENTS(memory_columns));

  icon = gdk_pixbuf_new_from_inline(-1, gear, FALSE, NULL);
  gtk_window_set_icon(GTK_WINDOW(ui->window), icon);
  gdk_pixbuf_unref(icon);

  gtk_widget_show(ui->window);
  ui_object_load(ui, object_file);

  g_log_set_handler(NULL,
                    G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
                    G_LOG_LEVEL_ERROR, log_handler, NULL);

  return ui;
}

void
ui_destroy(UI *ui)
{
  vm_destroy(ui->vm);
  g_object_unref(G_OBJECT(ui->gxml));

  g_free(ui);
}
