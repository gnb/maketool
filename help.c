#define DEBUG 1

#include <sys/types.h>
#include <gtk/gtk.h>
#include <string.h>
#include <malloc.h>
#include "spawn.h"
#if DEBUG
#include <stdio.h>
#endif

static GtkWidget	*about_shell = 0;
static GtkWidget	*about_make_shell = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/



static GtkWidget *
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

static GtkWidget *
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

static void
licence_cb(GtkWidget *w, gpointer data)
{
    fprintf(stderr, "Show licence...\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: add GPL text to doco & reference it from this window */

static const char about_str[] = "\
Maketool version 0.1\n\
\n\
(c) 1999 Greg Banks\n\
gnb@alphalink.com.au\n\
\n\
Maketool comes with ABSOLUTELY NO WARRANTY;\n\
for details press the Licence button. This is free\n\
software, and you are welcome to redistribute it\n\
under certain conditions; press the Licence button\n\
for details.\
";

void
help_about_show(GtkWidget *toplevel)
{
    if (about_shell == 0)
    {
	GtkWidget *label;

	about_shell = uiCreateOkDialog(toplevel, "About Maketool");

	/* TODO: logo */
	label = gtk_label_new(about_str);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_shell)->vbox), label);
	gtk_widget_show(label);

	uiDialogCreateButton(about_shell, "Licence...", licence_cb, (gpointer)0);
    }
    
    gtk_widget_show(about_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *make_version = 0;

static void
make_version_reap(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    if (!(WIFEXITED(status) || WIFSIGNALED(status)))
    	return;
	
    if (about_make_shell == 0)
    {
        GtkWidget *toplevel = GTK_WIDGET(user_data);
	GtkWidget *label;

	about_make_shell = uiCreateOkDialog(toplevel, "About Make");

	/* TODO: logo */
	label = gtk_label_new(make_version);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_make_shell)->vbox), label);
	gtk_widget_show(label);
    }

    gtk_widget_show(about_make_shell);
}

static void
make_version_input(int len, const char *buf, gpointer data)
{
    char *ptr;
    
    if (make_version == 0)
    {
    	ptr = make_version = (char *)malloc(len + 1);
    }
    else
    {
    	int oldlen = strlen(make_version);
	make_version = (char*)realloc(make_version, oldlen + len + 1);
	ptr = &make_version[oldlen];
    }
    
    memcpy(ptr, buf, len);
    ptr[len] = '\0';
    
#if DEBUG
    fprintf(stderr, "make_version_input(): make_version=\"%s\"\n", make_version);
#endif
}


void
help_about_make_show(GtkWidget *toplevel)
{
    /* TODO: grey out menu item while make --version running */
    if (make_version == 0)
	spawn_with_output("make --version", make_version_reap,
		make_version_input, (gpointer)toplevel);
    else if (about_make_shell != 0)
    	gtk_widget_show(about_make_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
