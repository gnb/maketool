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

#define DEFINE_GLOBALS
#include <stdarg.h>
#include "maketool.h"
#include "spawn.h"
#include "ui.h"
#include "log.h"
#include "util.h"
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

CVSID("$Id: main.c,v 1.27 1999-06-09 07:56:26 gnb Exp $");

typedef enum
{
    GR_NONE=-1,
    
    GR_NOTEMPTY=0,
    GR_NOTRUNNING,
    GR_RUNNING,
    GR_EDITABLE,
    GR_AGAIN,
    GR_ALL,
    GR_CLEAN,

    NUM_SETS
} Groups;

char		*targets;		/* targets on commandline */
const char	*last_target = 0;	/* last target built, for `again' */
GList		*predefined_targets = 0;	/* special well-known targets */
GList		*available_targets = 0;	/* all possible targets, for menu */
GtkWidget	*build_menu;
GtkWidget	*toolbar, *messagebox;
GtkWidget	*messageent;
pid_t		current_pid = -1;
gboolean	interrupted = FALSE;
gboolean	first_error = FALSE;

#define ANIM_MAX 15
GdkPixmap	*anim_pixmaps[ANIM_MAX+1];
GdkBitmap	*anim_masks[ANIM_MAX+1];
GtkWidget	*anim;


#define PASTE3(x,y,z) x##y##z

static void build_cb(GtkWidget *w, gpointer data);

#define g_list_find_str(l, s) \
	g_list_find_custom((l), (s), (GCompareFunc)strcmp)



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
message(const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    
    va_start(args, fmt);
    g_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    gtk_entry_set_text(GTK_ENTRY(messageent), buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
grey_menu_items(void)
{
    gboolean running = (current_pid > 0);
    gboolean empty = log_is_empty();
    LogRec *sel = log_selected();
    gboolean editable = (sel != 0 && sel->res.file != 0);
    gboolean again = (last_target != 0 && !running);
    gboolean all = (!running && g_list_find_str(available_targets, "all") != 0);
    gboolean clean = (!running && g_list_find_str(available_targets, "clean") != 0);

    ui_group_set_sensitive(GR_NOTRUNNING, !running);
    ui_group_set_sensitive(GR_RUNNING, running);
    ui_group_set_sensitive(GR_NOTEMPTY, !empty);
    ui_group_set_sensitive(GR_EDITABLE, editable);
    ui_group_set_sensitive(GR_AGAIN, again);
    ui_group_set_sensitive(GR_ALL, all);
    ui_group_set_sensitive(GR_CLEAN, clean);
}

char *
expand_prog(
    const char *prog,
    const char *file,
    int line,
    const char *target)
{
    int i;
    char *out;
    const char *expands[256];
    estring fflag;
    char linebuf[32];
    char runflags[32];
        
    for (i=0 ; i<256 ; i++)
    	expands[i] = 0;
    
    expands['f'] = file;

    if (line > 0)
    {
	sprintf(linebuf, "%d", line);
	expands['l'] = linebuf;
    }
    
    estring_init(&fflag);
    if (prefs.makefile != 0)
    {
    	estring_append_string(&fflag, "-f ");
	estring_append_string(&fflag, prefs.makefile);
	expands['m'] = fflag.data;
    }
    
    switch (prefs.run_how)
    {
    case RUN_SERIES:
    	break;	/* no flag */
    case RUN_PARALLEL_PROC:
    	sprintf(runflags, "-j%d", prefs.run_processes);
    	expands['p'] = runflags;
    	break;
    case RUN_PARALLEL_LOAD:
    	sprintf(runflags, "-l%.2g", prefs.run_load);
    	expands['p'] = runflags;
    	break;
    }
    
    if (prefs.ignore_failures)
    	expands['k'] = "-k";

    expands['v'] = prefs.var_make_flags;
    
    expands['t'] = target;
    
    out = expand_string(prog, expands);
    
    estring_free(&fflag);
    
    return out;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int anim_current = 0;
static gint anim_timer = -1;

static gint
anim_advance(gpointer data)
{
    if (anim_current++ == ANIM_MAX)
    	anim_current = 1;
    gtk_pixmap_set(GTK_PIXMAP(anim),
    	anim_pixmaps[anim_current], anim_masks[anim_current]);
    return TRUE;  /* keep going */
}

static void
anim_stop(void)
{
    if (anim_timer >= 0)
    {
	gtk_timeout_remove(anim_timer);
	anim_timer = -1;
    }
    gtk_pixmap_set(GTK_PIXMAP(anim),
    	anim_pixmaps[0], anim_masks[0]);
}

static void
anim_start(void)
{
    if (anim_timer < 0)
	anim_timer = gtk_timeout_add(200/* millisec */, anim_advance, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reap_list(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    estring *targs = (estring *)user_data;
    char *t, *buf;
    GList *predefs = 0, *list;

    if (!(WIFEXITED(status) || WIFSIGNALED(status)))
    	return;
	
#if DEBUG
    fprintf(stderr, "reaped list target\n");
#endif
    /* 
     * Parse the output of the program into whitespace-separated
     * strings which are targets. Build two lists, predefs (all
     * the found targets which are also in `predefined_targets')
     * and available_targets (all others).
     */
    buf = targs->data;
    while ((t = strtok(buf, " \t\r\n")) != 0)
    {
    	if (!strcmp(t, "-"))
	{
	    ui_add_separator(build_menu);	
	}
	else
	{
    	   t = g_strdup(t);
	   if (g_list_find_str(predefined_targets, t) != 0)
    	       predefs = g_list_append(predefs, t);
	   else
    	       available_targets = g_list_append(available_targets, t);
    	}
	buf = 0;
    }
    estring_free(targs);
    
    /*
     * Build two parts of the menu from the two lists. This
     * technique ensures the common and useful targets like
     * `all' and `install' will appear early in the menu and
     * will always be visible regardless of its size.
     */
    for (list = predefs ; list != 0 ; list = list->next)
	ui_add_button(build_menu, (const char*)list->data, build_cb, list->data,
		GR_NOTRUNNING);
    if (predefs != 0 && available_targets != 0)
	ui_add_separator(build_menu);
    for (list = available_targets ; list != 0 ; list = list->next)
	ui_add_button(build_menu, (const char*)list->data, build_cb, list->data,
		GR_NOTRUNNING);

    /*
     * Now prepend predefs to available_targets, which is now a
     * list of all the found targets, predefined or not.
     */
    available_targets = g_list_concat(predefs, available_targets);

    grey_menu_items();
}

static void
input_list(int len, const char *buf, gpointer data)
{
    estring *targs = (estring *)data;
    
    estring_append_chars(targs, buf, len);
}

static char *predef_targs[] = {
"all", "install", "uninstall",
"mostlyclean", "clean", "distclean", "reallyclean",
0
};

static void
list_targets(void)
{
    char *prog;
    pid_t pid;
    char **t;
    static estring targs;
    
    for (t = predef_targs ; *t ; t++)
	predefined_targets = g_list_prepend(predefined_targets, *t);
    
    estring_init(&targs);
    
    prog = expand_prog(prefs.prog_list_targets, 0, 0, 0);
    
    pid = spawn_with_output(prog, reap_list, input_list, (gpointer)&targs,
    	prefs.var_environment);
#if 0
    if (pid > 0)
    {
    	/* TODO: remove old menu items when Change Directory implemented */
    }
#endif
    g_free(prog);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
start_edit(LogRec *lr)
{
    char *prog;
    
    if (lr->res.file == 0 || lr->res.file[0] == '\0')
    	return;
	
    prog = expand_prog(prefs.prog_edit_source, lr->res.file, lr->res.line, 0);
    
    if (spawn_simple(prog, 0, 0) > 0)
	message(_("Editing %s (line %d)"), lr->res.file, lr->res.line);
	
    g_free(prog);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reap_make(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    char *target = (char *)user_data;
    
    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
    	char err_str[256];
    	char warn_str[256];
	char int_str[256];
	
	if (log_num_errors() > 0)
	    sprintf(err_str, _(", %d errors"), log_num_errors());
	else
	    err_str[0] = '\0';
	    
	if (log_num_warnings() > 0)
	    sprintf(warn_str, _(", %d warnings"), log_num_warnings());
	else
	    warn_str[0] = '\0';

	if (interrupted)
	    strcpy(int_str, _(" (interrupted)"));
	else
	    int_str[0] = '\0';
	    
	message(_("Finished making %s%s%s%s"), target, err_str, warn_str, int_str);
	
	current_pid = -1;
	log_end_build(target);
	anim_stop();
	grey_menu_items();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
handle_input(int len, const char *buf, gpointer data)
{
    static char linebuf[1024];
    static int nleftover = 0;
    LogRec *lr;
    
#if DEBUG   
    fprintf(stderr, "handle_data(): \"");
    fwrite(buf, len, 1, stderr);
    fprintf(stderr, "\"\n");
#endif    
    
    while (len > 0 && *buf)
    {
	char *p = strchr(buf, '\n');
	if (p == 0)
	{
    	    /* only a part of a line left - append to linebuf */
	    strncpy(&linebuf[nleftover], buf, sizeof(linebuf)-nleftover);
	    nleftover += len;
	    return;
	}
    	/* got an end-of-line - isolate the line & feed it to handle_line() */
	*p = '\0';
	strncpy(&linebuf[nleftover], buf, sizeof(linebuf)-nleftover);

	lr = log_add_line(linebuf);
	if (prefs.edit_first_error && first_error)
	{
	    if ((lr->res.code == FR_WARNING && prefs.edit_warnings) ||
	         lr->res.code == FR_ERROR)
	    {
	        first_error = FALSE;
	    	log_set_selected(lr);
		start_edit(lr);
	    }
	}
	
	nleftover = 0;
	len -= (p - buf);
	buf = ++p;
    }
}

void
build_start(const char *target)
{
    char *prog;
    
    prog = expand_prog(prefs.prog_make, 0, 0, target);
    
    current_pid = spawn_with_output(prog, reap_make, handle_input, (gpointer)target,
    	prefs.var_environment);
    if (current_pid > 0)
    {
	message(_("Making %s"), target);
	
	switch (prefs.start_action)
	{
	case START_NOTHING:
	    break;
	case START_CLEAR:
	    log_clear();
	    break;
	case START_COLLAPSE:
	    log_collapse_all();
	    break;
	}
	
	last_target = target;
	interrupted = FALSE;
	first_error = TRUE;
	log_start_build(prog);
	anim_start();
	grey_menu_items();
    }
    g_free(prog);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_exit_cb(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_open_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(_("Maketool: Open Log File"), log_open, "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_save_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(_("Maketool: Save Log File"), log_save, "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_again_cb(GtkWidget *w, gpointer data)
{
    if (last_target != 0 && current_pid < 0)
    	build_start(last_target);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_stop_cb(GtkWidget *w, gpointer data)
{
    if (current_pid > 0)
    {
    	interrupted = TRUE;
    	kill(current_pid, SIGKILL);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_cb(GtkWidget *w, gpointer data)
{
    build_start((char *)data);    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_widget_cb(GtkWidget *w, gpointer data)
{
    GtkWidget **wp = (GtkWidget **)data;
    
    if (GTK_CHECK_MENU_ITEM(w)->active)
    	gtk_widget_show(*wp);
    else
    	gtk_widget_hide(*wp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_flags_cb(GtkWidget *w, gpointer data)
{
    gboolean flags = GPOINTER_TO_INT(data);
    
    if (GTK_CHECK_MENU_ITEM(w)->active)
	log_set_flags(log_get_flags() | flags);
    else
	log_set_flags(log_get_flags() & ~flags);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_clear_cb(GtkWidget *w, gpointer data)
{
    log_clear();
}

static void
view_collapse_all_cb(GtkWidget *w, gpointer data)
{
    log_collapse_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_edit_cb(GtkWidget *w, gpointer data)
{
    LogRec *lr = log_selected();

    if (lr != 0)    
	start_edit(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_edit_next_cb(GtkWidget *w, gpointer data)
{
    gboolean next = GPOINTER_TO_INT(data);
    LogRec *lr = log_selected();
    
    do
    {
	lr = (next ? log_next_error(lr) : log_prev_error(lr));
    } while (lr != 0 && !(prefs.edit_warnings || lr->res.code == FR_ERROR));
    
    if (lr != 0)
    {	
	log_set_selected(lr);
	start_edit(lr);
    }
    else
    {
    	message(_("No more errors in log"));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_expand_cb(GtkCTree *tree, GtkCTreeNode *tree_node, gpointer data)
{
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, tree_node);

    lr->expanded = TRUE;    
}

static void
log_collapse_cb(GtkCTree *tree, GtkCTreeNode *tree_node, gpointer data)
{
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, tree_node);

    lr->expanded = FALSE;    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_click_cb(GtkCTree *tree, GtkCTreeNode *tree_node, gint column, gpointer data)
{
#if DEBUG
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, tree_node);
    
    fprintf(stderr, "log_click_cb: code=%d file=\"%s\" line=%d\n",
    	lr->res.code, lr->res.file, lr->res.line);
#endif
    grey_menu_items();
}

static void
log_doubleclick_cb(GtkWidget *w, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_2BUTTON_PRESS) 
    {
	LogRec *lr = log_selected();

	if (lr != 0)
	{
#if 0
	    fprintf(stderr, "log_doubleclick_cb: code=%d file=\"%s\" line=%d\n",
    		lr->res.code, lr->res.file, lr->res.line);
#endif
	    start_edit(lr);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
unimplemented(GtkWidget *w, gpointer data)
{
    fprintf(stderr, "Unimplemented\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ui_create_menus(GtkWidget *menubar)
{
    GtkWidget *menu;
    
    menu = ui_add_menu(menubar, _("File"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("Open Log..."), file_open_cb, 0, GR_NONE);
    ui_add_button(menu, _("Save Log..."), file_save_cb, 0, GR_NOTEMPTY);
    ui_add_separator(menu);
#if 0
    ui_add_button(menu, _("Change directory..."), unimplemented, 0, GR_NOTRUNNING);
    ui_add_separator(menu);
    ui_add_button(menu, _("Print"), unimplemented, 0, GR_NOTEMPTY);
    ui_add_button(menu, _("Print Settings..."), unimplemented, 0, GR_NONE);
    ui_add_separator(menu);
#endif
    ui_add_button(menu, _("Exit"), file_exit_cb, 0, GR_NONE);
    
    menu = ui_add_menu(menubar, _("Build"));
    build_menu = menu;
    ui_add_tearoff(menu);
    ui_add_button(menu, _("Again"), build_again_cb, 0, GR_AGAIN);
    ui_add_button(menu, _("Stop"), build_stop_cb, 0, GR_RUNNING);
    ui_add_separator(menu);
#if 0
    ui_add_button(menu, "all", build_cb, "all", GR_NOTRUNNING);
    ui_add_button(menu, "install", build_cb, "install", GR_NOTRUNNING);
    ui_add_button(menu, "clean", build_cb, "clean", GR_NOTRUNNING);
    ui_add_separator(menu);
    ui_add_button(menu, "random", build_cb, "random", GR_NOTRUNNING);
    ui_add_button(menu, "delay", build_cb, "delay", GR_NOTRUNNING);
    ui_add_button(menu, "targets", build_cb, "targets", GR_NOTRUNNING);
#endif
    
    menu = ui_add_menu(menubar, _("View"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("Clear Log"), view_clear_cb, 0, GR_NOTEMPTY);
    ui_add_button(menu, _("Collapse All"), view_collapse_all_cb, 0, GR_NOTEMPTY);
    ui_add_button(menu, _("Edit"), view_edit_cb, 0, GR_EDITABLE);
    ui_add_button(menu, _("Edit Next Error"), view_edit_next_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    ui_add_button(menu, _("Edit Prev Error"), view_edit_next_cb, GINT_TO_POINTER(FALSE), GR_NOTEMPTY);
    ui_add_separator(menu);
    ui_add_toggle(menu, _("Toolbar"), view_widget_cb, (gpointer)&toolbar, 0, TRUE);
    ui_add_toggle(menu, _("Statusbar"), view_widget_cb, (gpointer)&messagebox, 0, TRUE);
    ui_add_separator(menu);
    ui_add_toggle(menu, _("Errors"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_ERRORS),
    	0, log_get_flags() & LF_SHOW_ERRORS);
    ui_add_toggle(menu, _("Warnings"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_WARNINGS),
    	0, log_get_flags() & LF_SHOW_WARNINGS);
    ui_add_toggle(menu, _("Information"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_INFO),
    	0, log_get_flags() & LF_SHOW_INFO);
    ui_add_separator(menu);
    ui_add_button(menu, _("Preferences"), view_preferences_cb, 0, GR_NONE);
    
    menu = ui_add_menu_right(menubar, _("Help"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("About Maketool..."), help_about_cb, 0, GR_NONE);
    ui_add_button(menu, _("About make..."), help_about_make_cb, 0, GR_NONE);
#if 0
    ui_add_separator(menu);
    ui_add_button(menu, _("Help on..."), unimplemented, 0, GR_NONE);
    ui_add_separator(menu);
    ui_add_button(menu, _("Tutorial"), unimplemented, 0, GR_NONE);
    ui_add_button(menu, _("Reference Index"), unimplemented, 0, GR_NONE);
    ui_add_button(menu, _("Home Page"), unimplemented, 0, GR_NONE);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "all.xpm"
#include "again.xpm"
#include "clean.xpm"
#include "clear.xpm"
#include "next.xpm"
#include "print.xpm"

static void
ui_create_tools()
{
    char tooltip[1024];
    
    ui_tool_create(toolbar, _("Again"), _("Build last target again"),
    	again_xpm, build_again_cb, 0, GR_AGAIN);
    
    ui_tool_add_space(toolbar);

    sprintf(tooltip, _("Build `%s'"), "all");
    ui_tool_create(toolbar, "all", tooltip,
    	all_xpm, build_cb, "all", GR_ALL);
    sprintf(tooltip, _("Build `%s'"), "clean");
    ui_tool_create(toolbar, "clean", tooltip,
    	clean_xpm, build_cb, "clean", GR_CLEAN);
    
    ui_tool_add_space(toolbar);
    
    ui_tool_create(toolbar, _("Clear"), _("Clear log"),
    	clear_xpm, view_clear_cb, 0, GR_NOTEMPTY);
    ui_tool_create(toolbar, _("Next"), _("Edit next error or warning"),
    	next_xpm, view_edit_next_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    
    ui_tool_add_space(toolbar);
    
    ui_tool_create(toolbar, _("Print"), _("Print log"),
    	print_xpm, unimplemented, 0, GR_NOTEMPTY);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "anim0.xpm"
#include "anim0a.xpm"
#include "anim1.xpm"
#include "anim1a.xpm"
#include "anim1b.xpm"
#include "anim2.xpm"
#include "anim2a.xpm"
#include "anim3.xpm"
#include "anim3a.xpm"
#include "anim3b.xpm"
#include "anim4.xpm"
#include "anim4a.xpm"
#include "anim5.xpm"
#include "anim6.xpm"
#include "anim7.xpm"
#include "anim8.xpm"

#define ANIM_INIT(nm) \
    anim_pixmaps[n] = gdk_pixmap_create_from_xpm_d(toplevel->window, \
    	&anim_masks[n], 0, PASTE3(anim,nm,_xpm)); n++

static void
ui_init_anim_pixmaps(void)
{
    int n = 0;
    ANIM_INIT(0);
    ANIM_INIT(0a);
    ANIM_INIT(1);
    ANIM_INIT(1a);
    ANIM_INIT(1b);
    ANIM_INIT(2);
    ANIM_INIT(2a);
    ANIM_INIT(3);
    ANIM_INIT(3a);
    ANIM_INIT(3b);
    ANIM_INIT(4);
    ANIM_INIT(4a);
    ANIM_INIT(5);
    ANIM_INIT(6);
    ANIM_INIT(7);
    ANIM_INIT(8);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "maketool.xpm"

static void
ui_create(void)
{
    GtkWidget *table;
    GtkWidget *menubar, *logwin;
    GtkTooltips *tooltips;
    GtkWidget *sw;
    GdkPixmap *iconpm;
    GdkBitmap *iconmask = 0;
    static char *titles[1] = { "Log" };
    
    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    {
        char buf[512];
    	sprintf(buf, _("Maketool %s"), VERSION);
	gtk_window_set_title(GTK_WINDOW(toplevel), buf);
    }
    gtk_window_set_default_size(GTK_WINDOW(toplevel), 300, 500);
    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
    	GTK_SIGNAL_FUNC(file_exit_cb), NULL);
    gtk_container_border_width(GTK_CONTAINER(toplevel), 0);
    gtk_widget_show(GTK_WIDGET(toplevel));

    /* gtk_widget_realize(toplevel); */
    iconpm = gdk_pixmap_create_from_xpm_d(toplevel->window, &iconmask,
    		0, maketool_xpm);
    gdk_window_set_icon(toplevel->window, 0, iconpm, iconmask);
    gdk_window_set_icon_name(toplevel->window, "Maketool");

        
    tooltips = gtk_tooltips_new();

    table = gtk_table_new(4, 1, FALSE);
    gtk_container_add(GTK_CONTAINER(toplevel), table);
    gtk_container_border_width(GTK_CONTAINER(table), 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    
    menubar = gtk_menu_bar_new();
    gtk_table_attach(GTK_TABLE(table), menubar, 0, 1, 0, 1,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, 0,
	0, 0);
    ui_create_menus(menubar);
    gtk_widget_show(GTK_WIDGET(menubar));
        
    toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
    gtk_table_attach(GTK_TABLE(table), toolbar, 0, 1, 1, 2,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, 0,
	SPACING, 0);
    gtk_widget_show(GTK_WIDGET(toolbar));
    ui_create_tools(toolbar);
    
    sw = gtk_scrolled_window_new(0, 0);
    gtk_container_border_width(GTK_CONTAINER(sw), 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_table_attach(GTK_TABLE(table), sw, 0, 1, 2, 3,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, GTK_FILL|GTK_EXPAND|GTK_SHRINK,
	SPACING, 0);
    gtk_widget_show(GTK_WIDGET(sw));

    logwin = gtk_ctree_new_with_titles(1, 0, titles);
    gtk_ctree_set_line_style(GTK_CTREE(logwin), GTK_CTREE_LINES_NONE);
    gtk_ctree_set_expander_style(GTK_CTREE(logwin), GTK_CTREE_EXPANDER_TRIANGLE);
    gtk_clist_column_titles_hide(GTK_CLIST(logwin));
    gtk_ctree_set_indent(GTK_CTREE(logwin), 16);
    gtk_ctree_set_show_stub(GTK_CTREE(logwin), FALSE);
    gtk_clist_set_column_width(GTK_CLIST(logwin), 0, 400);
    gtk_clist_set_column_auto_resize(GTK_CLIST(logwin), 0, TRUE);
    gtk_signal_connect(GTK_OBJECT(logwin), "tree-select-row", 
    	GTK_SIGNAL_FUNC(log_click_cb), 0);
    gtk_signal_connect(GTK_OBJECT(logwin), "button_press_event", 
    	GTK_SIGNAL_FUNC(log_doubleclick_cb), 0);
    gtk_signal_connect(GTK_OBJECT(logwin), "tree_collapse", 
    	GTK_SIGNAL_FUNC(log_collapse_cb), 0);
    gtk_signal_connect(GTK_OBJECT(logwin), "tree_expand", 
    	GTK_SIGNAL_FUNC(log_expand_cb), 0);
    gtk_container_add(GTK_CONTAINER(sw), logwin);
    gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(sw),
    	GTK_CLIST(logwin)->hadjustment);
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(sw),
    	GTK_CLIST(logwin)->vadjustment);
    gtk_widget_show(logwin);
    log_init(logwin);
    
    messagebox = gtk_hbox_new(FALSE, SPACING);
    gtk_table_attach(GTK_TABLE(table), messagebox, 0, 1, 3, 4,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, 0,
	SPACING, 0);
    gtk_container_border_width(GTK_CONTAINER(messagebox), 0);
    gtk_widget_show(GTK_WIDGET(messagebox));
    
    messageent = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(messageent), FALSE);
    gtk_entry_set_text(GTK_ENTRY(messageent), _("Welcome to Maketool"));
    gtk_box_pack_start(GTK_BOX(messagebox), messageent, TRUE, TRUE, 0);   
    gtk_widget_show(messageent);
    
    ui_init_anim_pixmaps();

    anim = gtk_pixmap_new(anim_pixmaps[0], anim_masks[0]);
    gtk_box_pack_start(GTK_BOX(messagebox), anim, FALSE, FALSE, 0);   
    gtk_widget_show(anim);

    grey_menu_items();
    list_targets();

    gtk_widget_show(GTK_WIDGET(table));
    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
usage(void)
{
    fprintf(stderr, _("Usage: %s [options] target [target...]\n"), argv0);
    /* TODO: options */
}

static void
parse_args(int argc, char **argv)
{
    int i;
    estring targs;
    
    argv0 = argv[0];
    estring_init(&targs);
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
	    {
	    	printf(
"Maketool %s\n\
(c) 1999 Greg Banks <gnb@alphalink.com.au>\n\
Maketool comes with ABSOLUTELY NO WARRANTY.\n\
You may redistribute copies of this software\n\
under the terms of the GNU General Public\n\
Licence; see the file COPYING.\n",
		VERSION);
	    	exit(0);
	    }
	    /* TODO: options */
	    usage();
	}
	else
	{
	    if (targs.length > 0)
	    	estring_append_char(&targs, ' ');
	    estring_append_string(&targs, argv[i]);
	}
    }
    
    targets = targs.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    
    gtk_init(&argc, &argv);    
    preferences_init();
    parse_args(argc, argv);
    ui_create();
    if (targets != 0)
    	build_start(targets);
    gtk_main();
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
