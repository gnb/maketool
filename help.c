/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2000 Greg Banks
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
#include "task.h"
#include "ui.h"
#include "util.h"

CVSID("$Id: help.c,v 1.22 2000-07-29 15:18:14 gnb Exp $");

static GtkWidget	*licence_shell = 0;
static GtkWidget	*about_shell = 0;
static GtkWidget	*about_make_shell = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Hmm, the string literal from Hell. I hope this doesn't give
 * your compiler conniptions. Contact me if it does -- Greg
 */
static const char licence_str[] = 
#include "licence.c"
;

static void
licence_cb(GtkWidget *w, gpointer data)
{
    if (licence_shell == 0)
    {
	GtkWidget *hbox, *text, *sb;

	licence_shell = ui_create_ok_dialog(toplevel, _("Maketool: Licence"));
	ui_autonull_pointer(&licence_shell);
	gtk_widget_set_usize(licence_shell, 450, 300);

	hbox = gtk_hbox_new(FALSE, SPACING);
	gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(licence_shell)->vbox), hbox);
	gtk_widget_show(hbox);

	text = gtk_text_new(0, 0);
	gtk_text_set_editable(GTK_TEXT(text), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), text, TRUE, TRUE, 0);   
	gtk_widget_show(text);

	sb = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
	gtk_box_pack_start(GTK_BOX(hbox), sb, FALSE, FALSE, 0);   
	gtk_widget_show(sb);

	gtk_text_insert(GTK_TEXT(text), 0, 0, 0,
	    licence_str, sizeof(licence_str)-1);
    }

    gtk_widget_show(licence_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: add GPL text to doco & reference it from this window */

#include "maketool_l.xpm"

static const char about_str[] = "\
Maketool version %s\n\
\n\
(c) 1999-2000 Greg Banks\n\
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
	char *abt;

	about_shell = ui_create_ok_dialog(toplevel, _("Maketool: About"));
	ui_autonull_pointer(&about_shell);
	
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_shell)->vbox), hbox);
	gtk_widget_show(hbox);
	
	pm = gdk_pixmap_create_from_xpm_d(toplevel->window,
    		    &mask, 0, maketool_l_xpm);
	icon = gtk_pixmap_new(pm, mask);
	gtk_container_add(GTK_CONTAINER(hbox), icon);
	gtk_widget_show(icon);

	/* Build the string to display in the About box */
	abt = g_strdup_printf(_(about_str), VERSION);
	label = gtk_label_new(abt);
	g_free(abt);

	gtk_container_add(GTK_CONTAINER(hbox), label);
	gtk_widget_show(label);

	ui_dialog_create_button(about_shell, _("Licence..."), licence_cb, (gpointer)0);
    }
    
    gtk_widget_show(about_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "babygnu_l.xpm"
#define LOGO babygnu_l_xpm

static estring make_version = ESTRING_STATIC_INIT;

static void
make_version_input(Task *task, int len, const char *buf)
{
    estring_append_chars(&make_version, buf, len);
    
#if DEBUG
    fprintf(stderr, "make_version_input(): make_version=\"%s\"\n",
    	make_version.data);
#endif
}

static void
create_about_make_shell(void)
{
    GtkWidget *label;
    GtkWidget *icon;
    GtkWidget *hbox;
    GdkPixmap *pm;
    GdkBitmap *mask;

    about_make_shell = ui_create_ok_dialog(toplevel, _("Maketool: About Make"));
    ui_autonull_pointer(&about_make_shell);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_make_shell)->vbox), hbox);
    gtk_widget_show(hbox);

    pm = gdk_pixmap_create_from_xpm_d(toplevel->window,
    		&mask, 0, LOGO);
    icon = gtk_pixmap_new(pm, mask);
    gtk_container_add(GTK_CONTAINER(hbox), icon);
    gtk_widget_show(icon);

    label = gtk_label_new(make_version.data);
    gtk_container_add(GTK_CONTAINER(hbox), label);
    gtk_widget_show(label);
}

static void
make_version_reap(Task *task)
{
    if (about_make_shell == 0)
    	create_about_make_shell();

    gtk_widget_show(about_make_shell);
}

static TaskOps make_version_ops =
{
0,  	    	    	    	/* start */
make_version_input,	    	/* input */
make_version_reap, 	    	/* reap */
0   	    		    	/* destroy */
};

static Task *
make_version_task(void)
{
    return task_create(
    	(Task *)g_new(Task, 1),
	expand_prog(prefs.prog_list_version, 0, 0, 0),
	prefs.var_environment,
	&make_version_ops);
}

void
help_about_make_cb(GtkWidget *w, gpointer data)
{
    /* TODO: grey out menu item while make --version running */

    if (make_version.data == 0)
    {
    	/* 
	 * First time.  Haven't yet extracted version info from `make',
	 * so start `make' and delay creation of dialog box until we
	 * have the output, i.e. in the reap function.
	 */
    	task_spawn(make_version_task());
    }
    else
    {
    	if (about_make_shell == 0)
	    create_about_make_shell();
    	gtk_widget_show(about_make_shell);
    }
    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
