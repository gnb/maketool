/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2003 Greg Banks
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _UI_H_
#define _UI_H_

#include "common.h"
#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(2,0,0)
#define GTK2 1
#else
#define GTK2 0
#endif

#if GTK2
typedef void (*ui_callback_t)(GtkWidget *w, gpointer data);
#else
typedef GtkSignalFunc ui_callback_t;
#endif

void ui_widget_set_visible(GtkWidget *w, gboolean b);

int ui_combo_get_current(GtkWidget *);
void ui_combo_set_current(GtkWidget *, int);

void ui_clist_get_strings(GtkWidget*, int row, int ncols, char *text[]);
void ui_clist_set_strings(GtkWidget*, int row, int ncols, char *text[]);

void ui_group_add(guint group, GtkWidget *w);
void ui_group_set_sensitive(guint group, gboolean b);

GtkWidget *ui_create_file_sel(
    GtkWidget *parent,
    const char *title,
    void (*callback)(const char *filename),
    const char *filename);

/* toolbar stuff */
    
GtkWidget *ui_tool_create(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    ui_callback_t callback,
    gpointer user_data,
    gint group,
    const char *helptag);
void ui_tool_add_space(GtkWidget *toolbar);
GtkWidget *ui_tool_drop_menu_create(
    GtkWidget *toolbar,
    GtkWidget *linked_item, 	/* may be NULL */
    gint group,
    const char *helptag);
GtkWidget *ui_tool_drop_menu_get_menu(GtkWidget *);

/* Menubar stuff */    

void ui_set_accel_group(GtkAccelGroup *ag);
GtkWidget *ui_add_menu(GtkWidget *menubar, const char *label);
GtkWidget *ui_add_menu_right(GtkWidget *menubar, const char *label);
GtkWidget *ui_add_submenu(GtkWidget *menu, gboolean douline, const char *label);
GtkWidget *ui_add_button(
    GtkWidget *menu,
    const char *label,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group);
GtkWidget *ui_add_button_2(
    GtkWidget *menu,
    const char *label,
    gboolean douline,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group,
    gint position);
GtkWidget *ui_add_tearoff(GtkWidget *menu);
GtkWidget *ui_add_toggle(
    GtkWidget *menu,
    const char *label,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    GtkWidget *radio_other,
    gboolean set);
GtkWidget *ui_add_separator(GtkWidget *menu);
void ui_delete_menu_items(GtkWidget *menu);
int ui_container_num_visible_children(GtkContainer *con);

/* dialog stuff */

GtkWidget *ui_create_ok_dialog(
    GtkWidget *parent,
    const char *title);
GtkWidget *ui_create_apply_dialog(
    GtkWidget *parent,
    const char *title,
    ui_callback_t apply_cb,
    gpointer data);
GtkWidget *ui_create_dialog(
    GtkWidget *parent,
    const char *title);
GtkWidget *ui_dialog_create_button(
    GtkWidget *dialog,
    const char *label,
    ui_callback_t callback,
    gpointer user_data);
void ui_dialog_changed(GtkWidget *dialog);    

/* Returns a new unshown()n widget which deletes itself on user pressing OK */
GtkWidget *ui_message_dialog(GtkWidget *parent, const char *title,
    	    	    	     const char *msg);
GtkWidget *ui_message_dialog_f(GtkWidget *parent, const char *title,
    	    	    	     const char *fmt, ...)
#ifdef __GNUC__
__attribute__(( format(printf, 3, 4) ))
#endif
;
/* Show the given message dialog and wait until the user presses OK */
void ui_message_wait(GtkWidget *);


void ui_set_help_tag(GtkWidget *w, const char *tag);
const char *ui_get_help_tag(GtkWidget *w);
  
/* config stuff */
typedef struct
{
   char *name;
   int value;
} UiEnumRec;

/*
 * Initialise config database.  Returns number of variables
 * read from old persistent database, or -1 for error
 */
int ui_config_init(const char *pkg);
char *ui_config_get_string(const char *name, const char *defv);
int ui_config_get_int(const char *name, int defv);
int ui_config_get_enum(const char *name, int defv, UiEnumRec *enumdef);
int ui_config_get_flags(const char *name, int defv, UiEnumRec *enumdef);
gboolean ui_config_get_boolean(const char *name, gboolean defv);
void ui_config_set_string(const char *name, const char *val);
void ui_config_set_int(const char *name, int val);
void ui_config_set_enum(const char *name, int val, UiEnumRec *enumdef);
void ui_config_set_flags(const char *name, int val, UiEnumRec *enumdef);
void ui_config_set_boolean(const char *name, gboolean val);
void ui_config_backup(void);
void ui_config_sync(void);

/* uix.c */
unsigned long ui_windowid(GtkWidget *);

#endif /* _UI_H_ */
