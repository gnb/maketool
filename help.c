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

#include "maketool.h"
#include "spawn.h"
#include "ui.h"

CVSID("$Id: help.c,v 1.10 1999-05-30 11:24:39 gnb Exp $");

static GtkWidget	*about_shell = 0;
static GtkWidget	*about_make_shell = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
licence_cb(GtkWidget *w, gpointer data)
{
    fprintf(stderr, "Show licence...\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: add GPL text to doco & reference it from this window */

#include "maketool_l.xpm"

static const char about_str[] = "\
Maketool version %s\n\
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
help_about_cb(GtkWidget *w, gpointer data)
{
    if (about_shell == 0)
    {
	GtkWidget *label;
	GtkWidget *icon;
	GtkWidget *hbox;
	GdkPixmap *pm;
	GdkBitmap *mask;

	about_shell = uiCreateOkDialog(toplevel, _("About Maketool"));
	
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_shell)->vbox), hbox);
	gtk_widget_show(hbox);
	
	pm = gdk_pixmap_create_from_xpm_d(toplevel->window,
    		    &mask, 0, maketool_l_xpm);
	icon = gtk_pixmap_new(pm, mask);
	gtk_container_add(GTK_CONTAINER(hbox), icon);
	gtk_widget_show(icon);

	{
	    /* Build the string to display in the About box */
	    const char *fmt = _(about_str);
	    const char *version = VERSION;
	    char *buf = g_new(char, strlen(fmt) + strlen(version) + 1);
	    sprintf(buf, fmt, version);
	    label = gtk_label_new(buf);
	    g_free(buf);
	}
	gtk_container_add(GTK_CONTAINER(hbox), label);
	gtk_widget_show(label);

	uiDialogCreateButton(about_shell, _("Licence..."), licence_cb, (gpointer)0);
    }
    
    gtk_widget_show(about_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
make_version_reap(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    estring *vers = (estring *)data;

    if (!(WIFEXITED(status) || WIFSIGNALED(status)))
    	return;
	
    if (about_make_shell == 0)
    {
        GtkWidget *toplevel = GTK_WIDGET(user_data);
	GtkWidget *label;

	about_make_shell = uiCreateOkDialog(toplevel, _("About Make"));

	/* TODO: logo */
	label = gtk_label_new(vers->data);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_make_shell)->vbox), label);
	gtk_widget_show(label);
    }

    gtk_widget_show(about_make_shell);
}

static void
make_version_input(int len, const char *buf, gpointer data)
{
    estring *vers = (estring *)data;
    
    estring_append_chars(vers, buf, len);
    
#if DEBUG
    fprintf(stderr, "make_version_input(): make_version=\"%s\"\n", vers->data);
#endif
}


void
help_about_make_cb(GtkWidget *w, gpointer data)
{
    static estring vers;
    
    /* TODO: grey out menu item while make --version running */

    if (make_version == 0)
    {
	char *prog;
	prog = expand_prog(prefs.prog_list_version, 0, 0, 0);
	spawn_with_output(prog, make_version_reap,
		make_version_input, (gpointer)toplevel, &vers);
	g_free(prog);
    }
    else if (about_make_shell != 0)
    	gtk_widget_show(about_make_shell);
    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
