#include "ui.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GPtrArray *uiGroups = 0;

void
uiGroupAdd(gint group, GtkWidget *w)
{
    if (uiGroups == 0)
    	uiGroups = g_ptr_array_new();
	
    if (group >= uiGroups->len)
	g_ptr_array_set_size(uiGroups, group+1);
	
    g_ptr_array_index(uiGroups, group) = 
    	g_list_prepend(g_ptr_array_index(uiGroups, group), w);
}

void
uiGroupSetSensitive(gint group, gboolean b)
{
    GList *list;
    
    list = g_ptr_array_index(uiGroups, group);
    
    for ( ; list != 0 ; list = list->next)
    	gtk_widget_set_sensitive(GTK_WIDGET(list->data), b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define FILESEL_CALLBACK_DATA "uiCallback"

static void
uiFileSelOkCb(GtkWidget *w, gpointer data)
{
    GtkWidget *filesel = GTK_WIDGET(data);
    void (*callback)(const char *filename);
    
    callback = gtk_object_get_data(GTK_OBJECT(filesel), FILESEL_CALLBACK_DATA);
    (*callback)(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel)));
    gtk_widget_hide(filesel);
}

GtkWidget *
uiCreateFileSel(
    const char *title,
    void (*callback)(const char *filename),
    const char *filename)
{
    GtkWidget *filesel;
    
    filesel = gtk_file_selection_new(title);
    gtk_signal_connect(
	    GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
            "clicked", uiFileSelOkCb,
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

static GtkWidget *
_uiAddMenuAux(
    GtkWidget *menubar,
    const char *label,
    gboolean isRight)
{
    GtkWidget *menu, *item;

    item = gtk_menu_item_new_with_label(label);
    gtk_widget_show(item);

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    gtk_menu_bar_append(GTK_MENU_BAR(menubar), item);
    
    if (isRight)
    	gtk_menu_item_right_justify(GTK_MENU_ITEM(item));

    return menu;
}

GtkWidget *
uiAddMenu(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, FALSE);
}

GtkWidget *
uiAddMenuRight(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, TRUE);
}

GtkWidget *
uiAddButton(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    gint group)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(label);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    if (group >= 0)
    	uiGroupAdd(group, item);
    gtk_widget_show(item);
    return item;
}

GtkWidget *
uiAddTearoff(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_tearoff_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

GtkWidget *
uiAddToggle(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    GtkWidget *radioOther,
    gboolean set)
{
    GtkWidget *item;
    
    if (radioOther == 0)
    {
	item = gtk_check_menu_item_new_with_label(label);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), set);
    }
    else
    {
    	GSList *group;
	group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(radioOther));
	item = gtk_radio_menu_item_new_with_label(group, label);
    }
    
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    gtk_widget_show(item);
    return item;
}


GtkWidget *
uiAddSeparator(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GtkWidget *
uiToolCreate(
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
    	uiGroupAdd(group, item);
    return item;
}

void
uiToolAddSpace(GtkWidget *toolbar)
{
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GtkWidget *
uiDialogCreateButton(
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
uiDialogOkCb(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(GTK_WIDGET(data));
}

GtkWidget *
uiCreateOkDialog(
    GtkWidget *parent,
    const char *title)
{
    GtkWidget *dialog;
    
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    
    uiDialogCreateButton(dialog, "OK", uiDialogOkCb, (gpointer)dialog);
    
    return dialog;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
