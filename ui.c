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

#include "ui.h"

CVSID("$Id: ui.c,v 1.11 1999-06-13 13:37:55 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
ui_combo_get_current(GtkWidget *combo)
{
    GtkList *listw = GTK_LIST(GTK_COMBO(combo)->list);
    
    if (listw->selection == 0)
    	return 0;
    return gtk_list_child_position(listw, (GtkWidget*)listw->selection->data);
}

void
ui_combo_set_current(GtkWidget *combo, int n)
{
    gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list), n);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_clist_get_strings(GtkWidget *w, int row, int ncols, char *text[])
{
    int i;
    
    for (i=0 ; i<ncols ; i++)
	gtk_clist_get_text(GTK_CLIST(w), row, i, &text[i]);
}

void
ui_clist_set_strings(GtkWidget *w, int row, int ncols, char *text[])
{
    int i;
    
    for (i=0 ; i<ncols ; i++)
	gtk_clist_set_text(GTK_CLIST(w), row, i, text[i]);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GPtrArray *ui_groups = 0;

void
ui_group_add(gint group, GtkWidget *w)
{
    if (ui_groups == 0)
    	ui_groups = g_ptr_array_new();
	
    if (group >= ui_groups->len)
	g_ptr_array_set_size(ui_groups, group+1);
	
    g_ptr_array_index(ui_groups, group) = 
    	g_list_prepend(g_ptr_array_index(ui_groups, group), w);
}

void
ui_group_set_sensitive(gint group, gboolean b)
{
    GList *list;
    
    list = g_ptr_array_index(ui_groups, group);
    
    for ( ; list != 0 ; list = list->next)
    	gtk_widget_set_sensitive(GTK_WIDGET(list->data), b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define FILESEL_CALLBACK_DATA "ui_callback"

static void
ui_file_sel_ok_cb(GtkWidget *w, gpointer data)
{
    GtkWidget *filesel = GTK_WIDGET(data);
    void (*callback)(const char *filename);
    
    callback = gtk_object_get_data(GTK_OBJECT(filesel), FILESEL_CALLBACK_DATA);
    (*callback)(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel)));
    gtk_widget_hide(filesel);
}

GtkWidget *
ui_create_file_sel(
    const char *title,
    void (*callback)(const char *filename),
    const char *filename)
{
    GtkWidget *filesel;
    
    filesel = gtk_file_selection_new(title);
    gtk_signal_connect(
	    GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
            "clicked", ui_file_sel_ok_cb,
	    (gpointer)filesel);
    gtk_signal_connect_object(
	    GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
            "clicked", (GtkSignalFunc) gtk_widget_hide,
	    GTK_OBJECT(filesel));
    gtk_object_set_data(GTK_OBJECT(filesel), FILESEL_CALLBACK_DATA,
    	(gpointer)callback);
    if (filename != 0)
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), filename);

    return filesel;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define UI_MENU_ACCEL_GROUP	"ui_menu_accel_group"

static GtkAccelGroup *ui_accel_group = 0;

void
ui_set_accel_group(GtkAccelGroup *ag)
{
    ui_accel_group = ag;
}


static GtkWidget *
_ui_add_menu_aux(
    GtkWidget *menubar,
    const char *label,
    gboolean is_right)
{
    GtkWidget *menu, *item, *label_w;
    guint uline_key;
    
    label_w = gtk_accel_label_new(label);
    uline_key = gtk_label_parse_uline(GTK_LABEL(label_w), label);
    gtk_misc_set_alignment(GTK_MISC(label_w), 0.0, 0.5);
    gtk_widget_show(label_w);

    item = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(item), label_w);
    gtk_widget_show(item);
    
    gtk_widget_add_accelerator(item, "activate_item", 
				ui_accel_group, uline_key, GDK_MOD1_MASK, 0);

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    gtk_menu_bar_append(GTK_MENU_BAR(menubar), item);
    
    if (is_right)
    	gtk_menu_item_right_justify(GTK_MENU_ITEM(item));

    return menu;
}

GtkWidget *
ui_add_menu(GtkWidget *menubar, const char *label)
{
    return _ui_add_menu_aux(menubar, label, FALSE);
}

GtkWidget *
ui_add_menu_right(GtkWidget *menubar, const char *label)
{
    return _ui_add_menu_aux(menubar, label, TRUE);
}


static void
_ui_add_menu_accel(
    GtkWidget *menu,
    GtkWidget *item,
    guint uline_key)
{
    GtkAccelGroup *menu_accel_grp;
    GtkWidget *menu_shell;
 
    if (uline_key == 0)
    	return;

    menu_accel_grp = gtk_object_get_data(GTK_OBJECT(menu),
			    UI_MENU_ACCEL_GROUP);
    if (menu_accel_grp == 0)
    {
	menu_accel_grp = gtk_accel_group_new();
	menu_shell = gtk_widget_get_ancestor(menu, gtk_menu_shell_get_type());
	gtk_accel_group_attach(menu_accel_grp, GTK_OBJECT(menu_shell));
	gtk_object_set_data(GTK_OBJECT(menu), UI_MENU_ACCEL_GROUP,
	    	   	     menu_accel_grp);
    }
    gtk_widget_add_accelerator(item, "activate_item", 
			    menu_accel_grp, uline_key, 0, 0);
}


static void
_ui_add_accel(
    GtkWidget *menu,
    GtkWidget *item,
    const char *accel)
{
    GdkModifierType mods = 0;
    guint key = 0;

    if (ui_accel_group == 0)
    	return;

    gtk_accelerator_parse(accel, &key, &mods);
    gtk_accel_group_add(ui_accel_group,
	    key, mods, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(item), "activate");
}


GtkWidget *
ui_add_button_2(
    GtkWidget *menu,
    const char *label,
    gboolean douline,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group)
{
    GtkWidget *item;
    GtkWidget *label_w;
    guint uline_key = 0;
    
    label_w = gtk_accel_label_new(label);
    if (douline)
	 uline_key = gtk_label_parse_uline(GTK_LABEL(label_w), label);
    gtk_misc_set_alignment(GTK_MISC(label_w), 0.0, 0.5);
    gtk_widget_show(label_w);


    item = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(item), label_w);
    
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    if (group >= 0)
    	ui_group_add(group, item);
	
    /* Handle underline accelerator */
    if (douline)
	_ui_add_menu_accel(menu, item, uline_key);

    /* Handle accelerator */
    if (accel != 0)
	_ui_add_accel(menu, item, accel);

    gtk_widget_show(item);
    return item;
}

GtkWidget *
ui_add_button(
    GtkWidget *menu,
    const char *label,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group)
{
    return ui_add_button_2(menu, label, TRUE, accel, callback, calldata, group);
}


GtkWidget *
ui_add_tearoff(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_tearoff_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

GtkWidget *
ui_add_toggle(
    GtkWidget *menu,
    const char *label,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    GtkWidget *radio_other,
    gboolean set)
{
    GtkWidget *item, *label_w;
    guint uline_key;
    
    if (radio_other == 0)
    {
	item = gtk_check_menu_item_new();
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), set);
    }
    else
    {
    	GSList *group;
	group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(radio_other));
	item = gtk_radio_menu_item_new(group);
    }

    label_w = gtk_accel_label_new(label);
    uline_key = gtk_label_parse_uline(GTK_LABEL(label_w), label);
    gtk_misc_set_alignment(GTK_MISC(label_w), 0.0, 0.5);
    gtk_widget_show(label_w);

    gtk_container_add(GTK_CONTAINER(item), label_w);
    
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    
    /* Handle underline accelerator */
    _ui_add_menu_accel(menu, item, uline_key);

    /* Handle accelerator */
    if (accel != 0)
	_ui_add_accel(menu, item, accel);

    gtk_widget_show(item);    
    return item;
}


GtkWidget *
ui_add_separator(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GtkWidget *
ui_tool_create(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    GtkSignalFunc callback,
    gpointer user_data,
    gint group)
{
    GdkPixmap *pm = 0;
    GdkBitmap *mask = 0;
    GtkWidget *item;
    GdkWindow *win = gtk_widget_get_ancestor(toolbar, gtk_window_get_type())->window;


    pm = gdk_pixmap_create_from_xpm_d(win, &mask, 0, pixmap_xpm);
    item = gtk_toolbar_append_item(
    	GTK_TOOLBAR(toolbar),
    	name,
	tooltip,
	0,				/* tooltip_private_text */
    	gtk_pixmap_new(pm, mask),
	callback,
	user_data);
    if (group >= 0)
    	ui_group_add(group, item);
    return item;
}

void
ui_tool_add_space(GtkWidget *toolbar)
{
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GtkWidget *
ui_dialog_create_button(
    GtkWidget *dialog,
    const char *label,
    GtkSignalFunc callback,
    gpointer user_data)
{
    GtkWidget *button;
    
    button = gtk_button_new_with_label(label);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",  callback, user_data);
    gtk_widget_show(button);

    return button;
}

static void
ui_dialog_ok_cb(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(GTK_WIDGET(data));
}

GtkWidget *
ui_create_ok_dialog(
    GtkWidget *parent,
    const char *title)
{
    GtkWidget *dialog;
    
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    
    ui_dialog_create_button(dialog, _("OK"), ui_dialog_ok_cb, (gpointer)dialog);
    
    return dialog;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define APPLY_DIALOG_DATA "ui_apply_dialog"

typedef struct
{
    GtkWidget *dialog;
    GtkSignalFunc apply_cb;
    GtkWidget *ok_btn;
    GtkWidget *apply_btn;
    gpointer user_data;
} ApplyDialog;


static void
ui_apply_dialog_apply_cb(GtkWidget *w, gpointer data)
{
    ApplyDialog *ad = (ApplyDialog *)data;

    (*ad->apply_cb)(ad->dialog, ad->user_data);
    gtk_widget_set_sensitive(ad->ok_btn, FALSE);
    gtk_widget_set_sensitive(ad->apply_btn, FALSE);
}

static void
ui_apply_dialog_cancel_cb(GtkWidget *w, gpointer data)
{
    ApplyDialog *ad = (ApplyDialog *)data;

    gtk_widget_hide(ad->dialog);
}

static void
ui_apply_dialog_ok_cb(GtkWidget *w, gpointer data)
{
    ui_apply_dialog_apply_cb(w, data);
    ui_apply_dialog_cancel_cb(w, data);
}

GtkWidget *
ui_create_apply_dialog(
    GtkWidget *parent,
    const char *title,
    GtkSignalFunc apply_cb,
    gpointer data)
{
    ApplyDialog *ad;
    
    ad = g_new(ApplyDialog, 1);
    ad->apply_cb = apply_cb;
    ad->user_data = data;

    ad->dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(ad->dialog), title);

    gtk_object_set_data(GTK_OBJECT(ad->dialog), APPLY_DIALOG_DATA,
    	(gpointer)ad);
    
    ad->ok_btn = ui_dialog_create_button(ad->dialog, _("OK"), ui_apply_dialog_ok_cb, (gpointer)ad);
    gtk_widget_set_sensitive(ad->ok_btn, FALSE);
    ad->apply_btn = ui_dialog_create_button(ad->dialog, _("Apply"), ui_apply_dialog_apply_cb, (gpointer)ad);
    gtk_widget_set_sensitive(ad->apply_btn, FALSE);
    ui_dialog_create_button(ad->dialog, _("Cancel"), ui_apply_dialog_cancel_cb, (gpointer)ad);
    
    return ad->dialog;
}    

void
ui_dialog_changed(GtkWidget *dialog)
{
    ApplyDialog *ad = (ApplyDialog *)gtk_object_get_data(GTK_OBJECT(dialog), APPLY_DIALOG_DATA);

    gtk_widget_set_sensitive(ad->ok_btn, TRUE);
    gtk_widget_set_sensitive(ad->apply_btn, TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
