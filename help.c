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
#include <gdk/gdkkeysyms.h>

CVSID("$Id: help.c,v 1.26 2000-11-26 06:39:54 gnb Exp $");

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
	gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);
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
    gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);
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

#if ENABLE_ONLINE_HELP /* { */

static TaskOps help_browser_ops =
{
0,  	    	    	    	/* start */
0,  	    		    	/* input */
0,  	    	 	    	/* reap */
0   	    		    	/* destroy */
};

/*
 * Possible browser commands are (where %u is the url):
 * gnome-help-browser '%u'
 * gnome-moz-remote --newwin '%u'
 * netscape -remote 'openURL(%u)'
 * xterm -e lynx '%u'
 * ?? konqueror '%u'
 * ?? opera %u
 */

static Task *
help_browser_task(const char *url)
{
    char *command;
    
    /* TODO: expand from prefs */
    command = g_strdup_printf("gnome-help-browser %s", url);
    
    fprintf(stderr, "help_browser_task: command = \"%s\"\n", command);
    
    return task_create(
    	(Task *)g_new(Task, 1),
	/*expand_prog(prefs.prog_list_version, 0, 0, 0),*/
	command,
	/*environment*/0,
	&help_browser_ops);
}

static void
help_goto_url(const char *url)
{
    task_spawn(help_browser_task(url));
}

static char *
help_find_help_file(const char *name)
{
    char *file = 0;
    int i;
    static const char *locales[4];
    static char locale_lang_buf[3];
    
    if (locales[0] == 0)
    {
    	/* initialise searchlist of locales */
    	const char *loc;
	
    	loc = getenv("LC_MESSAGES");
	if (loc == 0)
    	    loc = getenv("LANG");

    	i = 0;
	if (loc != 0)
	{
	    locales[i++] = loc;
	    if (strlen(loc) > 2)
	    {
		locale_lang_buf[0] = loc[0];
		locale_lang_buf[1] = loc[1];
		locale_lang_buf[2] = '\0';
		locales[i++] = locale_lang_buf;
	    }
	}
	if (loc == 0 || strcmp(loc, "C"))
	    locales[i++] = "C";
	locales[i] = 0;
	assert(i < sizeof(locales)/sizeof(locales[0]));
	
#if 1
    	fprintf(stderr, "help_find_help_file: search locales:");
    	for (i=0 ; locales[i] != 0 ; i++)
	    fprintf(stderr, " \"%s\"", locales[i]);
    	fprintf(stderr, "\n");
#endif	
    }
    
    for (i=0 ; locales[i] != 0 ; i++)
    {
    	file = g_strconcat(HELPDIR "/", locales[i], "/", name, ".html", 0);
#if 1
	fprintf(stderr, "help_find_help_file: trying \"%s\"\n", file);
#endif
	if (file_exists(file))
	    break;
	g_free(file);
	file = 0;
    }
    
#if 1
    fprintf(stderr, "help_find_help_file: found \"%s\"\n", file);
#endif
    return file;
}


static void
help_goto_helpname(const char *name)
{
    char *file, *url;
    
    if ((file = help_find_help_file(name)) == 0)
    	return;
	
    url = g_strconcat("file:", file, 0);
    help_goto_url(url);
    g_free(file);
    g_free(url);
}

void
help_goto_helpname_cb(GtkWidget *w, gpointer data)
{
    help_goto_helpname((const char *)data);
}

void
help_goto_url_cb(GtkWidget *w, gpointer data)
{
    help_goto_url((const char *)data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "help_cursor.xbm"
#include "help_cursor_mask.xbm"

void
help_on_cb(GtkWidget *w, void *user_data)
{
    GdkEvent *event;
    const char *name;
    static GdkCursor *help_cursor = 0;
    
    if (help_cursor == 0)
    {
    	GdkBitmap *source, *mask;
	GdkColor fg, bg;
	
	source = gdk_bitmap_create_from_data(
	    toplevel->window,
	    help_cursor_bits,
	    help_cursor_width,
	    help_cursor_height);
	mask = gdk_bitmap_create_from_data(
	    toplevel->window,
	    help_cursor_mask_bits,
	    help_cursor_width,
	    help_cursor_height);
	fg.red = 0; fg.green = 0; fg.blue = 0; /* black */
	bg.red = 0xffff; bg.green = 0xffff; bg.blue = 0xffff; /* white */
    	help_cursor = gdk_cursor_new_from_pixmap(
	    source, mask,
	    &fg, &bg,
	    help_cursor_hot_x, help_cursor_hot_y);
	/* TODO: free source, mask */
    }

    /*
     * TODO: create a little toy window, pass it in as the 1st argument
     * to gdk_pointer_grab(), add an event handler to it, and use the
     * normal gtk main loop to wait for a button press event on it.
     */
    gdk_pointer_grab(toplevel->window,
    	/*owner_events*/TRUE,
	GDK_BUTTON_PRESS_MASK/*|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK*/,
	/*confine_to*/0,
	help_cursor,
	GDK_CURRENT_TIME);

    fprintf(stderr, "help_on_cb: about to get events\n");
    for (;;)
    {
	if ((event = gdk_event_get()) == 0)
	{
	    sleep(1);
	    continue;
	}
	fprintf(stderr, "help_on_cb: event->type = %d\n", event->type);
	
	if (event->type == GDK_BUTTON_PRESS)
	{
	    fprintf(stderr, "help_on_cb: Trying to find help...\n");
	    if ((w = gtk_get_event_widget(event)) != 0 &&
	    	(name = ui_get_help_name(w)) != 0)
		help_goto_helpname(name);
	    break;
	}
	/* TODO: do we need to free the event?? */
    }
    fprintf(stderr, "help_on_cb: finished getting events\n");
    
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
}

#endif /* } ENABLE_ONLINE_HELP*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
