/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999 Greg Banks
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


int ui_combo_get_current(GtkWidget *);
void ui_combo_set_current(GtkWidget *, int);

void ui_cListGetStrings(GtkWidget*, int row, int ncols, char *text[]);
void ui_cListSetStrings(GtkWidget*, int row, int ncols, char *text[]);

void ui_group_add(gint group, GtkWidget *w);
void ui_group_set_sensitive(gint group, gboolean b);

GtkWidget *ui_create_file_sel(
    const char *title,
    void (*callback)(const char *filename),
    const char *filename);

/* toolbar stuff */
    
GtkWidget *ui_tool_create(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    GtkSignalFunc callback,
    gpointer user_data,
    gint group);
void ui_tool_add_space(GtkWidget *toolbar);

/* Menubar stuff */    

void ui_set_accel_group(GtkAccelGroup *ag);
GtkWidget *ui_add_menu(GtkWidget *menubar, const char *label);
GtkWidget *ui_add_menu_right(GtkWidget *menubar, const char *label);
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
    gint group);
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

/* dialog stuff */

GtkWidget *ui_create_ok_dialog(
    GtkWidget *parent,
    const char *title);
GtkWidget *ui_create_apply_dialog(
    GtkWidget *parent,
    const char *title,
    GtkSignalFunc apply_cb,
    gpointer data);
GtkWidget *ui_dialog_create_button(
    GtkWidget *dialog,
    const char *label,
    GtkSignalFunc callback,
    gpointer user_data);
void ui_dialog_changed(GtkWidget *dialog);    
    
    
#endif /* _UI_H_ */
