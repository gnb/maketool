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

#include "maketool.h"
#include "filter.h"
#include "maketool_task.h"
#include "ui.h"
#include "util.h"
#include <gdk/gdkkeysyms.h>

CVSID("$Id: help.c,v 1.44 2003-09-29 01:07:20 gnb Exp $");

static GtkWidget	*licence_shell = 0;
static GtkWidget	*options_shell = 0;
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
	ui_set_help_tag(licence_shell, "licence-window");
	gtk_widget_set_usize(licence_shell, 590, 300);

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

static void
build_options_string(estring *e)
{
    int i;

    filter_describe_all(e, /*lod*/1, "    ");

    estring_append_string(e, "Makefile systems:\n");
    for (i = 0 ; makesystems[i] ; i++)
    	estring_append_printf(e, "    %s\n", _(makesystems[i]->label));

    estring_append_string(e, "Make programs:\n");
    for (i = 0 ; makeprograms[i] ; i++)
    	estring_append_printf(e, "    %s\n", _(makeprograms[i]->label));
}

static void
options_cb(GtkWidget *w, gpointer data)
{
    if (options_shell == 0)
    {
	GtkWidget *hbox, *text, *sb;
	estring options_str;

	options_shell = ui_create_ok_dialog(toplevel, _("Maketool: Compile Options"));
	ui_set_help_tag(options_shell, "compile-options-window");
	gtk_widget_set_usize(options_shell, 450, 300);

	hbox = gtk_hbox_new(FALSE, SPACING);
	gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(options_shell)->vbox), hbox);
	gtk_widget_show(hbox);

	text = gtk_text_new(0, 0);
	gtk_text_set_editable(GTK_TEXT(text), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), text, TRUE, TRUE, 0);   
	gtk_widget_show(text);

	sb = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
	gtk_box_pack_start(GTK_BOX(hbox), sb, FALSE, FALSE, 0);   
	gtk_widget_show(sb);

    	estring_init(&options_str);
	build_options_string(&options_str);

	gtk_text_insert(GTK_TEXT(text), 0, 0, 0,
	    options_str.data, options_str.length);

	estring_free(&options_str);
    }

    gtk_widget_show(options_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "maketool_l.xpm"

static const char warranty_str[] = N_("\
Maketool comes with ABSOLUTELY NO WARRANTY;\n\
for details press the Licence button. This is free\n\
software, and you are welcome to redistribute it\n\
under certain conditions; press the Licence button\n\
for details.\
");

static const char about_str[] = "\
Maketool version %s\n\
\n\
(c) 1999-2003 Greg Banks\n\
<gnb@alphalink.com.au>\n\
\n\
%s";

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
	ui_set_help_tag(about_shell, "about-maketool-window");
	
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
	abt = g_strdup_printf(about_str, VERSION, _(warranty_str));
	label = gtk_label_new(abt);
	g_free(abt);

	gtk_container_add(GTK_CONTAINER(hbox), label);
	gtk_widget_show(label);

	ui_dialog_create_button(about_shell, _("Options..."), options_cb, (gpointer)0);
	ui_dialog_create_button(about_shell, _("Licence..."), licence_cb, (gpointer)0);
    }
    
    gtk_widget_show(about_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static estring make_version = ESTRING_STATIC_INIT;
static const MakeProgram *make_version_makeprog;

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
    GtkWidget *hbox;
    char *title;

    title = g_strdup_printf(_("Maketool: About %s"), makeprog->label);
    about_make_shell = ui_create_ok_dialog(toplevel, title);
    ui_set_help_tag(about_make_shell, "about-make-window");
    g_free(title);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about_make_shell)->vbox), hbox);
    gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);
    gtk_widget_show(hbox);

    if (makeprog->logo_xpm != 0)
    {
	GdkPixmap *pm;
	GdkBitmap *mask;
	GtkWidget *icon;

	pm = gdk_pixmap_create_from_xpm_d(toplevel->window,
    		    &mask, 0, (char **)makeprog->logo_xpm);
	icon = gtk_pixmap_new(pm, mask);
	gtk_container_add(GTK_CONTAINER(hbox), icon);
	gtk_widget_show(icon);
    }

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
	&make_version_ops,
	0);
}

void
help_about_make_cb(GtkWidget *w, gpointer data)
{
    /* TODO: grey out menu item while make --version running */

    if (make_version.data == 0 ||
    	make_version_makeprog != makeprog)
    {
    	/* 
	 * First time, or makeprog has been changed since last time.
	 * Haven't yet extracted version info from `make',
	 * so start `make' and delay creation of dialog box until we
	 * have the output, i.e. in the reap function.
	 */
	if (about_make_shell != 0)
	{
	    gtk_widget_destroy(about_make_shell);
	    about_make_shell = 0;
	}
	estring_truncate(&make_version);
	make_version_makeprog = makeprog;
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

static gboolean help_initialised = FALSE;
#define HELP_NUM_LOCALES    4
static char *help_locales[HELP_NUM_LOCALES];
static GHashTable *help_indexes[HELP_NUM_LOCALES];


static void
help_init_locales(void)
{
    /* initialise searchlist of locales */
    const char *loc;
    int i;
    char langbuf[3];

    loc = getenv("LC_MESSAGES");
    if (loc == 0)
    	loc = getenv("LANG");

    i = 0;
    if (loc != 0)
    {
	help_locales[i++] = g_strdup(loc);
	if (strlen(loc) > 2 && loc[2] == '_')
	{
	    langbuf[0] = loc[0];
	    langbuf[1] = loc[1];
	    langbuf[2] = '\0';
	    help_locales[i++] = g_strdup(langbuf);
	}
    }
    if (loc == 0 || strcmp(loc, "C"))
	help_locales[i++] = g_strdup("C");
    help_locales[i] = 0;
    assert(i < HELP_NUM_LOCALES);

#if DEBUG > 2
    fprintf(stderr, "help_init_locales: search locales:");
    for (i=0 ; help_locales[i] != 0 ; i++)
	fprintf(stderr, " \"%s\"", help_locales[i]);
    fprintf(stderr, "\n");
#endif	
}

static GHashTable *
help_read_index(const char *filename)
{
    FILE *fp;
    char *tag, *urlpart;
    GHashTable *hash = 0;
    char buf[1024];
    static const char sep[] = " \t\n\r";

    if ((fp = fopen(filename, "r")) == 0)
    {
    	if (errno != ENOENT)
	    perror(filename);
	return 0;
    }
    

    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	if (buf[0] == '#')
	    continue;	/* skip comment lines */
    	if ((tag = strtok(buf, sep)) == 0)
	    continue;
    	if ((urlpart = strtok(0, sep)) == 0)
	    continue;
    	if (strtok(0, sep) != 0)
	    continue;

#if DEBUG > 2
	fprintf(stderr, "help_read_index: tag=\"%s\" urlpart=\"%s\"\n",
	    	    	tag, urlpart);
#endif

	if (hash == 0)
	    hash = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(hash, g_strdup(tag), g_strdup(urlpart));
    }
    
    fclose(fp);
    return hash;
}

static void
help_init_indexes(void)
{
    int i;
    char *file;

    for (i = 0 ; help_locales[i] != 0 ; i++)
    {
    	file = g_strconcat(HELPDIR "/", help_locales[i], "/index.dat", 0);
    	help_indexes[i] = help_read_index(file);
    	g_free(file);
    }
}

static void
help_init(void)
{
    if (help_initialised)
    	return;
    help_initialised = TRUE;
    help_init_locales();
    help_init_indexes();
}

/* Look up the given tag and return a file:/ URL or NULL */
static char *
help_lookup(const char *tag)
{
#define URLPREFIX   	"file://"
#define URLPREFIXLEN	(sizeof(URLPREFIX)-1)
    char *url = 0;
    int i;
    
    help_init();
    
    for (i = 0 ; help_locales[i] != 0 ; i++)
    {
    	/* first try to lookup the index for this locale */
    	if (help_indexes[i] != 0)
	{
	    char *urlpart;
	    char *filesep;
	    
	    if ((urlpart = g_hash_table_lookup(help_indexes[i], tag)) != 0)
	    {
    		url = g_strconcat(URLPREFIX HELPDIR "/", help_locales[i], "/", urlpart, 0);

	    	filesep = strrchr(url, '/');
		filesep = strchr(filesep, '#');
		if (filesep != 0)
		    *filesep = '\0';
		if (file_exists(url+URLPREFIXLEN))
		{
		    if (filesep != 0)
			*filesep = '#';
		    break;
		}
		g_free(url);
		url = 0;
	    }
	}

    	/* not found in index, maybe there's a file of that name */
    	url = g_strconcat(URLPREFIX HELPDIR "/", help_locales[i], "/", tag, ".html", 0);
#if DEBUG > 2
	fprintf(stderr, "help_lookup: trying \"%s\"\n", url);
#endif
	if (file_exists(url+URLPREFIXLEN))
	    break;
	g_free(url);
	url = 0;
    }
    
#if DEBUG > 2
    fprintf(stderr, "help_lookup: \"%s\" -> \"%s\"\n", tag, url);
#endif
    return url;
#undef URLPREFIX
#undef URLPREFIXLEN
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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
    const char *expands[256];
    
    memset(expands, 0, sizeof(expands));
    expands['u'] = url;
    
    return task_create(
    	(Task *)g_new(Task, 1),
	expand_string(prefs.prog_help_browser, expands),
	/*environment*/0,
	&help_browser_ops,
	0);
}

static void
help_goto_url(const char *url)
{
    task_spawn(help_browser_task(url));
}

static void
help_goto_tag(const char *tag)
{
    char *url;
    
    if ((url = help_lookup(tag)) != 0)
    {
	help_goto_url(url);
	g_free(url);
    }
}

void
help_goto_tag_cb(GtkWidget *w, gpointer data)
{
    help_goto_tag((const char *)data);
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
    const char *tag;
    static GdkCursor *help_cursor = 0;
    
    if (help_cursor == 0)
    {
    	GdkBitmap *source, *mask;
	GdkColor fg, bg;
	
	source = gdk_bitmap_create_from_data(
	    toplevel->window,
	    (char*)help_cursor_bits,
	    help_cursor_width,
	    help_cursor_height);
	mask = gdk_bitmap_create_from_data(
	    toplevel->window,
	    (char*)help_cursor_mask_bits,
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

#if DEBUG > 2
    fprintf(stderr, "help_on_cb: about to get events\n");
#endif
    for (;;)
    {
	if ((event = gdk_event_get()) == 0)
	{
	    sleep(1);
	    continue;
	}
#if DEBUG > 2
	fprintf(stderr, "help_on_cb: event->type = %d\n", event->type);
#endif
	
	if (event->type == GDK_BUTTON_PRESS)
	{
#if DEBUG > 2
	    fprintf(stderr, "help_on_cb: Trying to find help...\n");
#endif
	    if ((w = gtk_get_event_widget(event)) != 0 &&
	    	(tag = ui_get_help_tag(w)) != 0)
		help_goto_tag(tag);
	    break;
	}
	else if (event->type == GDK_ENTER_NOTIFY ||
	    	 event->type == GDK_LEAVE_NOTIFY ||
	    	 event->type == GDK_EXPOSE)
	{
	    /* pass these on to GTK so it can highlight buttons etc */
	    gtk_main_do_event(event);
	}
	/* TODO: do we need to free the event?? */
    }
#if DEBUG > 2
    fprintf(stderr, "help_on_cb: finished getting events\n");
#endif

    gdk_pointer_ungrab(GDK_CURRENT_TIME);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
