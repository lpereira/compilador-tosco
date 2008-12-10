/*
 * Simple Pascal Compiler
 * Configuration Manager
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#include "conf.h"

static gchar	*conf_file = NULL;
static GKeyFile	*key_file = NULL;
static gboolean	conf_changed = FALSE;

static void _conf_dump_file(void)
{
    gchar *f;

    if ((f = g_key_file_to_data(key_file, NULL, NULL))) {
	g_file_set_contents(conf_file, f, -1, NULL);
	g_free(f);
    }
}

static gboolean _conf_dump_timeout(gpointer data)
{
    if (conf_changed) {
        conf_changed = FALSE;
        _conf_dump_file();
    }
    
    return FALSE;
}

void conf_init(char *program_name)
{
    gchar *dir, *file, *hidden_name, *conf_name;
    
    hidden_name = g_strdup_printf(".%s", program_name);
    conf_name = g_strdup_printf("%s.conf", program_name);

    dir = g_build_filename(g_get_home_dir(), hidden_name, NULL);
    if (!g_file_test(dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
      if (g_rmdir(dir) == -1)  {
        g_remove(dir);
      }
      
      g_mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    
    file = g_build_filename(dir, conf_name, NULL);
    if (!conf_file && !key_file) {
        conf_file = g_strdup(file);

        key_file = g_key_file_new();
        g_key_file_load_from_file(key_file, file, 0, NULL);
        
        g_atexit(_conf_dump_file);
    } else {
        g_warning("conf_init() already called; file is '%s'", conf_file);
    }

    g_free(file);
    g_free(dir);
    g_free(conf_name);
    g_free(hidden_name);
}

static void _conf_changed(void)
{
    conf_changed = TRUE;
    g_timeout_add(5000, _conf_dump_timeout, NULL);
}

static void _conf_link_checkbutton(GtkWidget * widget, gpointer data)
{
    ConfLink *cl = (ConfLink *) data;
    gboolean new_value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    conf_save_boolean(cl->group, cl->key, new_value);

    if (cl->update_fn) {
        cl->update_fn(cl->user_data, GINT_TO_POINTER(new_value));
    }
    
    _conf_changed();
}

static void _conf_link_spinbutton(GtkWidget * widget, gpointer data)
{
    ConfLink *cl = (ConfLink *) data;
    gint new_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

    conf_save_integer(cl->group, cl->key, new_value);
    
    if (cl->update_fn) {
        cl->update_fn(cl->user_data, GINT_TO_POINTER(new_value));
    }

    _conf_changed();
}

static void _conf_link_fontbutton(GtkWidget * widget, gpointer data)
{
    ConfLink *cl = (ConfLink *) data;
    gchar *new_value = (char*)gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget));
    
    conf_save_string(cl->group, cl->key, new_value);
    
    if (cl->update_fn) {
        cl->update_fn(cl->user_data, new_value);
    }
  
    _conf_changed();
}

void conf_link_fontbutton(GtkFontButton * fontbutton,
                          char *group, char *key, char *def,
                          ConfUpdateFunction update_fn, gpointer user_data)
{
    ConfLink *cl;
    gchar *font;

    cl = g_new0(ConfLink, 1);
    cl->group = group;
    cl->key = key;
    cl->update_fn = update_fn;
    cl->user_data = user_data;

    g_signal_connect(G_OBJECT(fontbutton), "font-set",
		     G_CALLBACK(_conf_link_fontbutton), cl);

    font = conf_get_string(group, key, def);
    gtk_font_button_set_font_name(fontbutton, font);
    g_free(font);

    _conf_link_fontbutton(GTK_WIDGET(fontbutton), cl);
}

static void _conf_link_filebutton(GtkWidget * widget, gpointer data)
{
    ConfLink *cl = (ConfLink *) data;
    gchar *new_value = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    
    conf_save_string(cl->group, cl->key, new_value);
    
    if (cl->update_fn) {
        cl->update_fn(cl->user_data, new_value);
    }
  
    _conf_changed();
    g_free(new_value);
}

void conf_link_filebutton(GtkFileChooserButton * filebutton,
                          char *group, char *key, char *def,
                          ConfUpdateFunction update_fn, gpointer user_data)
{
    ConfLink *cl;
    gchar *file;

    cl = g_new0(ConfLink, 1);
    cl->group = group;
    cl->key = key;
    cl->update_fn = update_fn;
    cl->user_data = user_data;

#if GTK_CHECK_VERSION(2,11,0)
    g_signal_connect(G_OBJECT(filebutton), "file-set",
		     G_CALLBACK(_conf_link_filebutton), cl);
#else
    g_signal_connect(G_OBJECT(filebutton), "selection-changed",
		     G_CALLBACK(_conf_link_filebutton), cl);
#endif

    file = conf_get_string(group, key, def);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filebutton), file);
    g_free(file);
}


void conf_link_checkbutton(GtkCheckButton * checkbutton,
			   char *group, char *key, gboolean def,
                           ConfUpdateFunction update_fn, gpointer user_data)
{
    ConfLink *cl;

    cl = g_new0(ConfLink, 1);
    cl->group = group;
    cl->key = key;
    cl->update_fn = update_fn;
    cl->user_data = user_data;

    g_signal_connect(G_OBJECT(checkbutton), "toggled",
		     G_CALLBACK(_conf_link_checkbutton), cl);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton),
				 conf_get_boolean(group, key, def));

    _conf_link_checkbutton(GTK_WIDGET(checkbutton), cl);
}

static void _conf_link_combobox(GtkWidget * widget, gpointer data)
{
    ConfLink *cl = (ConfLink *) data;
    gint new_value = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

    conf_save_integer(cl->group, cl->key, new_value);
    
    if (cl->update_fn) {
        cl->update_fn(cl->user_data, GINT_TO_POINTER(new_value));
    }

    _conf_changed();
}

void conf_link_combobox(GtkComboBox * combobox, char *group, char *key, gint def,
                        ConfUpdateFunction update_fn, gpointer user_data)
{
    ConfLink *cl;

    cl = g_new0(ConfLink, 1);
    cl->group = group;
    cl->key = key;
    cl->update_fn = update_fn;
    cl->user_data = user_data;

    g_signal_connect(G_OBJECT(combobox), "changed",
		     G_CALLBACK(_conf_link_combobox), cl);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox),
			     conf_get_integer(group, key, def));

    _conf_link_combobox(GTK_WIDGET(combobox), cl);
}

void conf_link_spinbutton(GtkSpinButton * spinbutton,
			  char *group, char *key, gint def,
                          ConfUpdateFunction update_fn, gpointer user_data)
{
    ConfLink *cl;

    cl = g_new0(ConfLink, 1);
    cl->group = group;
    cl->key = key;
    cl->update_fn = update_fn;
    cl->user_data = user_data;
    
    g_signal_connect(G_OBJECT(spinbutton), "value-changed",
		     G_CALLBACK(_conf_link_spinbutton), cl);
    gtk_spin_button_set_value(spinbutton, conf_get_integer(group, key, def));

    _conf_link_spinbutton(GTK_WIDGET(spinbutton), cl);
}

void conf_save_integer(char *group, char *key, gint value)
{
    g_key_file_set_integer(key_file, group, key, value);
}

gint conf_get_integer(char *group, char *key, gint def)
{
    GError	*error = NULL;
    gint	retval;
    
    retval = g_key_file_get_integer(key_file, group, key, &error);
    if (error) {
        g_error_free(error);
        return def;
    }
    
    return retval;
}

void conf_save_string(char *group, char *key, gchar * value)
{
    g_key_file_set_string(key_file, group, key, value ? value : "");
}

gchar *conf_get_string(char *group, char *key, char *def)
{
    GError	*error = NULL;
    gchar	*retval;
    
    retval = g_key_file_get_string(key_file, group, key, &error);
    if (error) {
        g_error_free(error);
        g_free(retval);
        return g_strdup(def);
    }
    
    return retval;
}

void conf_save_boolean(char *group, char *key, gboolean value)
{
    g_key_file_set_boolean(key_file, group, key, value);
}

gboolean conf_get_boolean(char *group, char *key, gboolean def)
{
    GError	*error = NULL;
    gboolean 	retval;
    
    retval = g_key_file_get_boolean(key_file, group, key, &error);
    if (error) {
        g_error_free(error);
        return def;
    }
    
    return retval;
}
