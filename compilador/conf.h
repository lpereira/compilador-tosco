/*
 * Simple Pascal Compiler
 * Configuration Manager
 *
 * Copyright (c) 2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */

#ifndef __CONF_H__
#define __CONF_H__

#include <glib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

typedef void    (*ConfUpdateFunction)      (gpointer user_data, gpointer new_value);
typedef struct _ConfLink ConfLink;

struct _ConfLink {
    gchar		*group, *key;
    ConfUpdateFunction	update_fn;
    gpointer		user_data;
};

void		conf_init(char *file);

void		conf_save_integer(char *group, char *key, gint value);
gint		conf_get_integer(char *group, char *key, gint def);

void		conf_save_string(char *group, char *key, gchar * value);
gchar		*conf_get_string(char *group, char *key, char *def);
void		conf_save_boolean(char *group, char *key, gboolean value);
gboolean	conf_get_boolean(char *group, char *key, gboolean def);

void		conf_link_checkbutton(GtkCheckButton * checkbutton, char *group,
				      char *key, gboolean def,
				      ConfUpdateFunction update_fn, gpointer user_data);
void		conf_link_spinbutton(GtkSpinButton * spinbutton, char *group,
				      char *key, gint def,
				      ConfUpdateFunction update_fn, gpointer user_data);
void		conf_link_combobox(GtkComboBox * combobox, char *group,
				   char *key, gboolean def,
				   ConfUpdateFunction update_fn, gpointer user_data);
void		conf_link_fontbutton(GtkFontButton * fontbutton,
                	             char *group, char *key, char *def,
	                             ConfUpdateFunction update_fn, gpointer user_data);
void		conf_link_filebutton(GtkFileChooserButton * filebutton,
				     char *group, char *key, char *def,
	                             ConfUpdateFunction update_fn, gpointer user_data);

#endif				/* __CONF_H__ */
