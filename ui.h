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


int uiComboGetCurrent(GtkWidget *);
void uiComboSetCurrent(GtkWidget *, int);

void uiCListGetStrings(GtkWidget*, int row, int ncols, char *text[]);
void uiCListSetStrings(GtkWidget*, int row, int ncols, char *text[]);

void uiGroupAdd(gint group, GtkWidget *w);
void uiGroupSetSensitive(gint group, gboolean b);

GtkWidget *uiCreateFileSel(
    const char *title,
    void (*callback)(const char *filename),
    const char *filename);

/* toolbar stuff */
    
GtkWidget *uiToolCreate(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    GtkSignalFunc callback,
    gpointer user_data,
    gint group);
void uiToolAddSpace(GtkWidget *toolbar);

/* Menubar stuff */    

GtkWidget *uiAddMenu(GtkWidget *menubar, const char *label);
GtkWidget *uiAddMenuRight(GtkWidget *menubar, const char *label);
GtkWidget *uiAddButton(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group);
GtkWidget *uiAddTearoff(GtkWidget *menu);
GtkWidget *uiAddToggle(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    GtkWidget *radioOther,
    gboolean set);
GtkWidget *uiAddSeparator(GtkWidget *menu);

/* dialog stuff */

GtkWidget *uiCreateOkDialog(
    GtkWidget *parent,
    const char *title);
GtkWidget *uiCreateApplyDialog(
    GtkWidget *parent,
    const char *title,
    GtkSignalFunc apply_cb,
    gpointer data);
GtkWidget *uiDialogCreateButton(
    GtkWidget *dialog,
    const char *label,
    GtkSignalFunc callback,
    gpointer user_data);
void uiDialogChanged(GtkWidget *dialog);    
    
    
#endif /* _UI_H_ */
