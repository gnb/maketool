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

#include "ui.h"
#include "util.h"

CVSID("$Id: ui.c,v 1.35 2003-09-24 10:28:46 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_widget_set_visible(GtkWidget *w, gboolean b)
{
    if (b)
    	gtk_widget_show(w);
    else
    	gtk_widget_hide(w);
}

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

static void
ui_group_destroy_cb(GtkWidget *w, gpointer user_data)
{
    gint group = GPOINTER_TO_INT(user_data);
    
    g_ptr_array_index(ui_groups, group) = 
    	g_list_remove(g_ptr_array_index(ui_groups, group), w);
}

void
ui_group_add(guint group, GtkWidget *w)
{
    if (ui_groups == 0)
    	ui_groups = g_ptr_array_new();
	
    if (group >= ui_groups->len)
	g_ptr_array_set_size(ui_groups, group+1);
	
    g_ptr_array_index(ui_groups, group) = 
    	g_list_prepend(g_ptr_array_index(ui_groups, group), w);
	
    gtk_signal_connect(GTK_OBJECT(w), "destroy",
    	    GTK_SIGNAL_FUNC(ui_group_destroy_cb), GINT_TO_POINTER(group));
}

void
ui_group_set_sensitive(guint group, gboolean b)
{
    GList *list;
    
    if (ui_groups == 0 || group >= ui_groups->len)
    	return;
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
    
    callback = (void (*)(const char*))gtk_object_get_data(
    	    	    	    GTK_OBJECT(filesel), FILESEL_CALLBACK_DATA);
    (*callback)(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel)));
    gtk_widget_hide(filesel);
}

GtkWidget *
ui_create_file_sel(
    GtkWidget *parent,
    const char *title,
    void (*callback)(const char *filename),
    const char *filename)
{
    GtkWidget *filesel;
    
    filesel = gtk_file_selection_new(title);
    gtk_window_set_position(GTK_WINDOW(filesel), GTK_WIN_POS_CENTER);
    gtk_window_set_transient_for(GTK_WINDOW(filesel),
    	    GTK_WINDOW(parent));
    gtk_signal_connect(
	    GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
            "clicked", GTK_SIGNAL_FUNC(ui_file_sel_ok_cb),
	    (gpointer)filesel);
    gtk_signal_connect_object(
	    GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
            "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
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

#if !GTK2

static GtkWidget *
_ui_create_label(
    GtkWidget *item,
    const char *label,
    guint *uline_key_p)
{
    GtkWidget *label_w;
    
    label_w = gtk_accel_label_new(label);
    if (uline_key_p != 0)
	 *uline_key_p = gtk_label_parse_uline(GTK_LABEL(label_w), label);
    gtk_misc_set_alignment(GTK_MISC(label_w), 0.0, 0.5);
    gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label_w), item);
    gtk_container_add(GTK_CONTAINER(item), label_w);
    gtk_widget_show(label_w);
    return label_w;
}    

#endif /* !GTK2 */

static GtkWidget *
_ui_add_menu_aux(
    GtkWidget *parent,	    /* menubar or menu */
    const char *label,
    gboolean douline,
    gboolean is_right,
    gint position)
{
    GtkWidget *menu, *item;
#if !GTK2
    guint uline_key = 0;
#endif
    
#if GTK2
    if (douline)
	item = gtk_menu_item_new_with_mnemonic(label);
    else
	item = gtk_menu_item_new_with_label(label);
#else /* !GTK2 */
    item = gtk_menu_item_new();
    _ui_create_label(item, label, (douline ? &uline_key : 0));
#endif /* !GTK2 */
    gtk_widget_show(item);
    
#if !GTK2
    if (douline && uline_key != 0)
	gtk_widget_add_accelerator(item, "activate", 
				ui_accel_group, uline_key, GDK_MOD1_MASK, 0);
#endif /* !GTK2 */

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    if (GTK_IS_MENU_BAR(parent))
    {
    	if (position < 0)
	    gtk_menu_bar_append(GTK_MENU_BAR(parent), item);
	else
	    gtk_menu_bar_insert(GTK_MENU_BAR(parent), item, position);
    }
    else
    {
    	if (position < 0)
	    gtk_menu_append(GTK_MENU(parent), item);
	else
	    gtk_menu_insert(GTK_MENU(parent), item, position);
    }
    
    if (is_right)
    	gtk_menu_item_right_justify(GTK_MENU_ITEM(item));

    return menu;
}

GtkWidget *
ui_add_menu(GtkWidget *menubar, const char *label)
{
    return _ui_add_menu_aux(menubar, label, TRUE, FALSE, -1);
}

GtkWidget *
ui_add_menu_right(GtkWidget *menubar, const char *label)
{
    return _ui_add_menu_aux(menubar, label, TRUE, TRUE, -1);
}

GtkWidget *
ui_add_submenu(GtkWidget *menu, gboolean douline, const char *label)
{
    return _ui_add_menu_aux(menu, label, douline, FALSE, -1);
}

#if !GTK2

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
	menu_shell = gtk_widget_get_ancestor(menu, gtk_menu_shell_get_type());
	menu_accel_grp = gtk_accel_group_new();
	gtk_accel_group_attach(menu_accel_grp, GTK_OBJECT(menu_shell));
	gtk_object_set_data(GTK_OBJECT(menu), UI_MENU_ACCEL_GROUP,
	    	   	     menu_accel_grp);
    }
    gtk_widget_add_accelerator(item, "activate", 
			    menu_accel_grp, uline_key, 0, 0);
}
#endif /* !GTK2 */


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
    gtk_widget_add_accelerator(item, "activate", 
			    ui_accel_group, key, mods, GTK_ACCEL_VISIBLE);
#if 0
    gtk_accel_group_add(ui_accel_group,
	    key, mods, GTK_ACCEL_VISIBLE,
	    GTK_OBJECT(item), "activate");
#endif
}


GtkWidget *
ui_add_button_2(
    GtkWidget *menu,
    const char *label,
    gboolean douline,
    const char *accel,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group,
    gint position)
{
    GtkWidget *item;
#if !GTK2
    guint uline_key = 0;
#endif

#if GTK2
    if (douline)
	item = gtk_menu_item_new_with_mnemonic(label);
    else
	item = gtk_menu_item_new_with_label(label);
#else
    item = gtk_menu_item_new();
    _ui_create_label(item, label, (douline ? &uline_key : 0));
#endif

    if (position < 0)
	gtk_menu_append(GTK_MENU(menu), item);
    else
	gtk_menu_insert(GTK_MENU(menu), item, position);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    if (group >= 0)
    	ui_group_add(group, item);
	
#if !GTK2
    /* Handle underline accelerator */
    if (douline)
	_ui_add_menu_accel(menu, item, uline_key);
#endif

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
    return ui_add_button_2(menu, label, TRUE, accel, callback, calldata, group, -1);
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
    GtkWidget *item;
#if !GTK2
    guint uline_key;
#endif
    
#if GTK2
    if (radio_other == 0)
    {
	item = gtk_check_menu_item_new_with_mnemonic(label);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), set);
    }
    else
    {
    	GSList *group;
	group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(radio_other));
	item = gtk_radio_menu_item_new_with_mnemonic(group, label);
    }
#else /* !GTK2 */
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

    _ui_create_label(item, label, &uline_key);
#endif /* !GTK2 */
    
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    
#if !GTK2
    /* Handle underline accelerator */
    _ui_add_menu_accel(menu, item, uline_key);
#endif /* !GTK2 */

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

void
ui_delete_menu_items(GtkWidget *menu)
{
    GList *list, *next;
    
    for (list = gtk_container_children(GTK_CONTAINER(menu)) ;
    	 list != 0 ;
	 list = next)
    {
    	GtkWidget *child = (GtkWidget *)list->data;
	next = list->next;

    	if (GTK_IS_TEAROFF_MENU_ITEM(child))
	    continue;
	    
	gtk_widget_destroy(child);	
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
ui_container_num_visible_children(GtkContainer *con)
{
    GList *kids = gtk_container_children(con);
    int nvis = 0;
    
    while (kids != 0)
    {
    	if (GTK_WIDGET_VISIBLE(kids->data))
	    nvis++;
    	kids = g_list_remove_link(kids, kids);
    }
    
    return nvis;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GtkWidget *
ui_tool_create(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    ui_callback_t callback,
    gpointer user_data,
    gint group,
    const char *helpname)
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
    	GTK_SIGNAL_FUNC(callback),
	user_data);
    if (group >= 0)
    	ui_group_add(group, item);
    if (helpname != 0)
    	ui_set_help_name(item, helpname);

    return item;
}

void
ui_tool_add_space(GtkWidget *toolbar)
{
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* prevents the dialog being inconveniently destroyed rather than hidden */
static void
ui_dialog_delete_cb(GtkWidget *w, void *user_data)
{
    gtk_widget_hide(w);
}


GtkWidget *
ui_create_dialog(
    GtkWidget *parent,
    const char *title)
{
    GtkWidget *dialog, *bbox;
    
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),
    	    GTK_WINDOW(parent));
    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 4);
    gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", 
    	GTK_SIGNAL_FUNC(ui_dialog_delete_cb), 0);

    bbox = gtk_hbutton_box_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), bbox);
    gtk_widget_show(bbox);

    return dialog;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GtkWidget *
ui_dialog_create_button(
    GtkWidget *dialog,
    const char *label,
    ui_callback_t callback,
    gpointer user_data)
{
    GtkWidget *button, *bbox;
    
    button = gtk_button_new_with_label(label);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);    
    bbox = ((GtkBoxChild *)GTK_BOX(GTK_DIALOG(dialog)->action_area)->children->data)->widget;
    gtk_box_pack_start(GTK_BOX(bbox), button,
    	    	      /*expand*/FALSE, /*fill*/TRUE, /*padding*/0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	    	       GTK_SIGNAL_FUNC(callback), user_data);
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
    GtkWidget *dialog, *btn;
    
    dialog = ui_create_dialog(parent, title);
    
    btn = ui_dialog_create_button(dialog, _("OK"), ui_dialog_ok_cb, (gpointer)dialog);
    ui_set_help_name(btn, "ok");
    gtk_window_set_default(GTK_WINDOW(dialog), btn);
    return dialog;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define APPLY_DIALOG_DATA "ui_apply_dialog"

typedef struct
{
    GtkWidget *dialog;
    ui_callback_t apply_cb;
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
    gtk_window_set_default(GTK_WINDOW(ad->dialog), ad->apply_btn);
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
    ui_callback_t apply_cb,
    gpointer data)
{
    ApplyDialog *ad;
    GtkWidget *btn;
    
    ad = g_new(ApplyDialog, 1);
    ad->apply_cb = apply_cb;
    ad->user_data = data;

    ad->dialog = ui_create_dialog(parent, title);

    gtk_object_set_data(GTK_OBJECT(ad->dialog), APPLY_DIALOG_DATA,
    	(gpointer)ad);
    
    ad->ok_btn = ui_dialog_create_button(ad->dialog, _("OK"), ui_apply_dialog_ok_cb, (gpointer)ad);
    gtk_widget_set_sensitive(ad->ok_btn, FALSE);
    ui_set_help_name(ad->ok_btn, "ok");
    ad->apply_btn = ui_dialog_create_button(ad->dialog, _("Apply"), ui_apply_dialog_apply_cb, (gpointer)ad);
    gtk_widget_set_sensitive(ad->apply_btn, FALSE);
    ui_set_help_name(ad->apply_btn, "apply");
    btn = ui_dialog_create_button(ad->dialog, _("Cancel"), ui_apply_dialog_cancel_cb, (gpointer)ad);
    ui_set_help_name(btn, "cancel");
    gtk_window_set_default(GTK_WINDOW(ad->dialog), btn);
    
    return ad->dialog;
}    

void
ui_dialog_changed(GtkWidget *dialog)
{
    ApplyDialog *ad = (ApplyDialog *)gtk_object_get_data(GTK_OBJECT(dialog), APPLY_DIALOG_DATA);

    gtk_widget_set_sensitive(ad->ok_btn, TRUE);
    gtk_widget_set_sensitive(ad->apply_btn, TRUE);
    gtk_window_set_default(GTK_WINDOW(ad->dialog), ad->ok_btn);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#include "gnome-info.xpm"

static void
ui_message_dialog_hide_cb(GtkWidget *w, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

GtkWidget *
ui_message_dialog(GtkWidget *parent, const char *title, const char *msg)
{
    GtkWidget *shell;
    GtkWidget *hbox;
    GtkWidget *icon;
    GtkWidget *label;
    static GdkPixmap *pm;
    static GdkBitmap *mask;

    shell = ui_create_ok_dialog(parent, title);
    gtk_signal_connect(GTK_OBJECT(shell), "hide",
    	    	       GTK_SIGNAL_FUNC(ui_message_dialog_hide_cb), shell);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_border_width(GTK_CONTAINER(hbox), 4);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(shell)->vbox), hbox);
    gtk_widget_show(hbox);

    if (pm == 0)
    {
	pm = gdk_pixmap_create_from_xpm_d(parent->window,
    		    &mask, 0, gnome_info_xpm);
    }
    icon = gtk_pixmap_new(pm, mask);
    gtk_container_add(GTK_CONTAINER(hbox), icon);
    gtk_widget_show(icon);

    label = gtk_label_new(msg);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_container_add(GTK_CONTAINER(hbox), label);
    gtk_widget_show(label);
    
    return shell;
}

GtkWidget *
ui_message_dialog_f(GtkWidget *parent, const char *title, const char *fmt, ...)
{
    va_list args;
    char *msg;
    GtkWidget *w;
    
    va_start(args, fmt);
    msg = g_strdup_vprintf(fmt, args);
    va_end(args);

    w = ui_message_dialog(parent, title, msg);
    
    g_free(msg);

    return w;
}

void
ui_message_wait(GtkWidget *w)
{
    gtk_widget_show(w);
    
    do
    {
    	gtk_main_iteration();
    }
    while (GTK_WIDGET_VISIBLE(w));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char _ui_help_key[] = "ui-help-key";

void
ui_set_help_name(GtkWidget *w, const char *str)
{
    gtk_object_set_data(GTK_OBJECT(w), _ui_help_key, (void*)str);
}

const char *
ui_get_help_name(GtkWidget *w)
{
    while (w != 0)
    {
	const char *name = (const char *)gtk_object_get_data(GTK_OBJECT(w), _ui_help_key);
	if (name != 0)
	    return name;
	w = w->parent;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GHashTable *ui_config_data = 0;
static char *ui_config_filename = 0;

static void
ui_config_set_take_value(const char *name, char *value)
{
    gpointer orig_key = 0, orig_value = 0;

    if (g_hash_table_lookup_extended(ui_config_data, name, &orig_key, &orig_value))
    {
	g_hash_table_remove(ui_config_data, orig_key);
    	g_free(orig_key);
	g_free(orig_value);
    }

    if (value != 0)
	g_hash_table_insert(ui_config_data, g_strdup(name), value);
#if DEBUG
    fprintf(stderr, "ui_config_set: %s = \"%s\"\n", name, value);
#endif
}

static void
ui_config_set(const char *name, const char *value)
{
    ui_config_set_take_value(name, g_strdup(value));
}


void
ui_config_init(const char *pkg)
{
    estring name, value, filename;
    FILE *fp;
    int c;

    ui_config_data = g_hash_table_new(g_str_hash, g_str_equal);

    estring_init(&name);
    estring_init(&value);

    estring_init(&filename);
    estring_append_string(&filename, getenv("HOME"));
    estring_append_string(&filename, "/.");
    estring_append_string(&filename, pkg);
    estring_append_string(&filename, "rc");
    ui_config_filename = filename.data;
    
    if ((fp = fopen(ui_config_filename, "r")) == 0)
    {
    	if (errno != ENOENT)
	    perror(ui_config_filename);
	return;
    }

    for (;;) 
    {
   	c = fgetc(fp);
	if (c == EOF)
	    break;

   	/* skip comments */
	while (c == '#')
	{
	     while ((c = fgetc(fp)) != '\n')
	     	;
	     c = fgetc(fp);
	}

	/* name */
	estring_truncate(&name);
	estring_append_char(&name, c);
	while ((c = fgetc(fp)) != '=' && c != '\n')
	    estring_append_char(&name, c);
	if (c == '\n')
	    ungetc(c, fp);
	
	/* value */
	estring_truncate(&value);
	/* swallow c == '=' */
	while ((c = fgetc(fp)) != '\n')
	    estring_append_char(&value, c);

	/* got name, value */
	ui_config_set(name.data, value.data);
    }
   
    fclose(fp);
    estring_free(&name);
    estring_free(&value);
}

static UiEnumRec ui_boolean_enum_def[] = {
{"TRUE",	TRUE},
{"FALSE",	FALSE},
{"true",	TRUE},
{"false",	FALSE},
{"yes",		TRUE},
{"no",		FALSE},
{"on",		TRUE},
{"off",		FALSE},
{"1",		TRUE},
{"0",		FALSE},
{0, 0}};

char *
ui_config_get_string(const char *name, const char *defv)
{
    const char *v;
    
    v = (const char *)g_hash_table_lookup(ui_config_data, name);
    if (v == 0)
    	v = defv;
    return g_strdup(v);
}

int
ui_config_get_int(const char *name, int defv)
{
    char *v;
    
    v = (char *)g_hash_table_lookup(ui_config_data, name);
    return (v == 0 ? defv : atoi(v));
}

int
ui_config_get_enum(const char *name, int defv, UiEnumRec *enumdef)
{
    char *v;
    
    v = (char *)g_hash_table_lookup(ui_config_data, name);
    if (v != 0)
    {
	for ( ; enumdef->name != 0 ; enumdef++)
    	    if (!strcmp(enumdef->name, v))
		return enumdef->value;
		
	if (isdigit(v[0]))
	    return atoi(v);
    }
    return defv;
}

int
ui_config_get_flags(const char *name, int defv, UiEnumRec *enumdef)
{
    char *v;
    int val = 0;
    char **p, **parts;
    UiEnumRec *er;
    
    v = (char *)g_hash_table_lookup(ui_config_data, name);
    if (v == 0)
    	return defv;
	
    parts = g_strsplit(v, ",", 1024);
    for (p = parts ; *p ; p++)
    {
	if (isdigit((*p)[0]))
	{
	    val |= atoi(*p);
	    continue;
	}
	
	for (er = enumdef ; er->name != 0 ; er++)
	{
    	    if (!strcmp(er->name, *p))
	    {
	    	val |= er->value;
		break;
	    }
    	}
    }
    g_strfreev(parts);
    return val;
}


gboolean
ui_config_get_boolean(const char *name, gboolean defv)
{
    return (gboolean)ui_config_get_enum(name, (int)defv, ui_boolean_enum_def);
}

void
ui_config_set_string(const char *name, const char *val)
{
    ui_config_set(name, val);
}

void
ui_config_set_int(const char *name, int val)
{
    char buf[256];
    
    sprintf(buf, "%d", val);
    ui_config_set(name, buf);
}

void
ui_config_set_enum(const char *name, int val, UiEnumRec *enumdef)
{
    char *v = 0, buf[256];
    
    for ( ; enumdef->name != 0 ; enumdef++)
    	if (enumdef->value == val)
	{
	    v = enumdef->name;
	    break;
	}
    if (v == 0)
    {
    	sprintf(buf, "%d", val);
	v = buf;
    }
    
    ui_config_set(name, v);
}

void
ui_config_set_flags(const char *name, int val, UiEnumRec *enumdef)
{
    estring stringval;
    
    estring_init(&stringval);
    
    for ( ; val != 0 && enumdef->name != 0 ; enumdef++)
    {
    	if (enumdef->value & val)
	{
	    if (stringval.length > 0)
	    	estring_append_char(&stringval, ',');
	    estring_append_string(&stringval, enumdef->name);
	    val &= ~enumdef->value;
	}
    }
    if (val != 0)
    {
	if (stringval.length > 0)
	    estring_append_char(&stringval, ',');
    	estring_append_printf(&stringval, "%d", val);
    }
    
    ui_config_set_take_value(name, stringval.data);
}


void
ui_config_set_boolean(const char *name, gboolean val)
{
    ui_config_set_enum(name, (val ? TRUE : FALSE), ui_boolean_enum_def);
}


void
ui_config_backup(void)
{
    char *bakfile;
    
    bakfile = g_strconcat(ui_config_filename, ".OLD", 0);
    if (rename(ui_config_filename, bakfile) < 0)
    	perror(bakfile);
    g_free(bakfile);
}

static void
ui_config_save_one(gpointer keyp, gpointer valuep, gpointer user_data)
{
    FILE *fp = (FILE *)user_data;
    const char *key = (const char *)keyp;
    const char *value = (const char *)valuep;
    
    fprintf(fp, "%s=%s\n", key, value);
}

void
ui_config_sync(void)
{
    FILE *fp;
   
    if ((fp = fopen(ui_config_filename, "w")) == 0)
    {
   	 perror(ui_config_filename);
   	 return;
    }
    fputs("# Written by maketool. Do not edit\n", fp);
    g_hash_table_foreach(ui_config_data, ui_config_save_one, (gpointer)fp);
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
