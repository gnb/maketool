#ifndef _UI_H_
#define _UI_H_

#include <gtk/gtk.h>


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
GtkWidget *uiDialogCreateButton(
    GtkWidget *dialog,
    const char *label,
    GtkSignalFunc callback,
    gpointer user_data);
    
    
#endif /* _UI_H_ */
