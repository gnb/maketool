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

#define DEFINE_GLOBALS
#include <stdarg.h>
#include "maketool.h"
#include "ui.h"
#include "log.h"
#include "util.h"
#include <ctype.h>
#include "task.h"
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

CVSID("$Id: main.c,v 1.58 2000-07-21 07:35:32 gnb Exp $");

typedef enum
{
    GR_NONE=-1,
    
    GR_NOTEMPTY=0,
    GR_NOTRUNNING,
    GR_CLEAR_LOG,   	/* !running && !empty */
    GR_RUNNING,
    GR_SELECTED,
    GR_EDITABLE,
    GR_AGAIN,
    GR_ALL,
    GR_CLEAN,
    GR_FIND_AGAIN,
    GR_NEVER,	    	/* never active */

    NUM_SETS
} Groups;


/*
 * How amazingly confusing that make and X11 selections
 * both use the terminology `target' for completely
 * different concepts. Sigh.
 */
typedef enum
{
    SEL_TARGET_STRING,
    SEL_TARGET_TEXT,
    SEL_TARGET_COMPOUND_TEXT
} SelectionTargets;


char		**cmd_targets;		/* targets on commandline */
int 	    	cmd_num_targets;
const char	*last_target = 0;	/* last target built, for `again' */
GList		*available_targets = 0;	/* all possible targets, for menu */
GtkWidget	*build_menu;
GtkWidget	*toolbar_hb, *messagebox;
GtkWidget	*messageent;
gboolean	interrupted = FALSE;
gboolean	first_error = TRUE;

#define ANIM_MAX 15
GdkPixmap	*anim_pixmaps[ANIM_MAX+1];
GdkBitmap	*anim_masks[ANIM_MAX+1];
GtkWidget	*anim;
GtkWidget   	*toolbar;
GtkWidget   	*again_menu_item, *again_tool_item;

GdkAtom     	clipboard_atom = GDK_NONE;
char  	    	*clipboard_text = 0;
GtkWidget   	*clipboard_widget;

GtkWidget   	*dir_previous_menu;
#define MAX_DIR_HISTORY 16
GList	    	*dir_history;	    	/* of strings */

/*
 * These are the targets specifically mentioned in the
 * current GNU makefile standards (except `mostlyclean'
 * which is from the old standards). These targets are
 * visually separated in the Build menu.
 */
static const char *standard_targets[] = {
"all",
"install", "install-strip", "installcheck", "installdirs", "uninstall",
"mostlyclean", "clean", "distclean", "reallyclean", "maintainer-clean",
"TAGS", "tags",
"info", "dvi",
"dist",
"check",
0
};

#define PASTE3(x,y,z) x##y##z

static void build_cb(GtkWidget *w, gpointer data);
static void handle_input(int len, const char *buf);
static void handle_line(const char *line);
static void set_main_title(void);
static void construct_build_menu_basic_items(void);
static void dir_previous_cb(GtkWidget *w, gpointer data);

#define g_list_find_str(l, s) \
	g_list_find_custom((l), (s), (GCompareFunc)strcmp)

/*
 * Number of non-standard targets to be present before
 * the build menu should be split up into multiple submenus.
 */
#define BUILD_MENU_THRESHOLD	20

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
message(const char *fmt, ...)
{
    va_list args;
    char *msg;
    
    va_start(args, fmt);
    msg = g_strdup_vprintf(fmt, args);
    va_end(args);
    
    gtk_entry_set_text(GTK_ENTRY(messageent), msg);
    g_free(msg);
}

/*
 * Force the last changed message to appear
 * immediately instead of delaying until the
 * next dip into the main loop. Useful when
 * a long-blocking action is about to be done.
 */
void
message_flush(void)
{
    while (g_main_pending())
    	g_main_iteration(/*may_block*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
grey_menu_items(void)
{
    gboolean running = task_is_running();
    gboolean empty = log_is_empty();
    LogRec *sel = log_selected();
    gboolean selected = (sel != 0);
    gboolean editable = (sel != 0 && sel->res.file != 0);
    gboolean again = (last_target != 0 && !running);
    gboolean all = (!running && g_list_find_str(available_targets, "all") != 0);
    gboolean clean = (!running && g_list_find_str(available_targets, "clean") != 0);

    ui_group_set_sensitive(GR_NOTRUNNING, !running);
    ui_group_set_sensitive(GR_CLEAR_LOG, !running && !empty);
    ui_group_set_sensitive(GR_RUNNING, running);
    ui_group_set_sensitive(GR_NOTEMPTY, !empty);
    ui_group_set_sensitive(GR_SELECTED, selected);
    ui_group_set_sensitive(GR_EDITABLE, editable);
    ui_group_set_sensitive(GR_AGAIN, again);
    ui_group_set_sensitive(GR_ALL, all);
    ui_group_set_sensitive(GR_CLEAN, clean);
    ui_group_set_sensitive(GR_FIND_AGAIN, !empty && find_can_find_again());
    ui_group_set_sensitive(GR_NEVER, FALSE);
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
    	if (prefs.run_processes > 0)
    	    sprintf(runflags, "-j%d", prefs.run_processes);
	else
    	    strcpy(runflags, "-j");
    	expands['p'] = runflags;
    	break;
    case RUN_PARALLEL_LOAD:
    	if (prefs.run_load > 0)
    	    sprintf(runflags, "-l%.2g", (gfloat)prefs.run_load / 10.0);
	else
    	    strcpy(runflags, "-l");
    	expands['p'] = runflags;
    	break;
    }
    
    if (prefs.ignore_failures)
    	expands['k'] = "-k";

    expands['v'] = prefs.var_make_flags;
    
    expands['t'] = target;
    
    if (prefs.dryrun)
	expands['n'] = "-n";
    
    out = expand_string(prog, expands);
    
    estring_free(&fflag);
    
    return out;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
set_last_target(const char *target)
{
    GtkLabel *label;
    char *menulabel;
    char *tooltip;
    
    last_target = target;
    
    /* Note: this assumes that the accelerator remains constant, i.e. Ctrl+A,
     *       even though the position of the underscore in the label
     *       may change.
     */
    menulabel = g_strdup_printf(_("_Again (%s)"), last_target);
    label = GTK_LABEL(GTK_BIN(again_menu_item)->child);
    gtk_label_set_text(label, menulabel);
    gtk_label_parse_uline(label, menulabel);
    g_free(menulabel);

    tooltip = g_strdup_printf(_("Build `%s' again"), last_target);
    gtk_tooltips_set_tip(GTK_TOOLBAR(toolbar)->tooltips, again_tool_item,
    	tooltip, 0);
    g_free(tooltip);
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
/*
 * These 2 fns are called when the task queue
 * is started or emptied.
 */

static void
work_started(void)
{
    anim_start();
    grey_menu_items();
}

static void
work_ended(void)
{
    anim_stop();
    grey_menu_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
make_makefile_start(Task *task)
{
    log_start_build(task->command);
}

static void
make_makefile_input(Task *task, int len, const char *buf)
{
    handle_input(len, buf);
}

static void
make_makefile_reap(Task *task)
{
    handle_input(0, 0);
    log_end_build(task->command);
}

static TaskOps make_makefile_ops =
{
make_makefile_start,  	   	/* start */
make_makefile_input,	    	/* input */
make_makefile_reap, 	    	/* reap */
0   	    	    	    	/* destroy */
};

static Task *
make_makefile_task(void)
{
    return task_create(
    	(Task *)g_new(Task, 1),
	expand_prog(prefs.prog_make_makefile, 0, 0, 0),
	prefs.var_environment,
	&make_makefile_ops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
append_build_menu_items(GList *list)
{
    GtkWidget *menu = 0;
    char *targ;
    int n;
    gboolean multiple_mode = (g_list_length(list) > BUILD_MENU_THRESHOLD);
    
    for (n = 0 ; list != 0 ; list = list->next)
    {
    	targ = (char *)list->data;

    	if (multiple_mode)
	{
	    if (n == 0)
	    {
	    	GList *last;
		
		last = g_list_nth(list, BUILD_MENU_THRESHOLD-1);
		if (last == 0)
		    last = g_list_last(list);
		if (last != list)
		{
		    char *label = g_strdup_printf("%s - %s", targ, (const char *)last->data);
#if DEBUG
	    	    fprintf(stderr, "creating Build submenu \"%s\"\n", label);
#endif
		    menu = ui_add_submenu(build_menu, /*douline*/FALSE, label);
		    g_free(label);
		}
		else
		    menu = build_menu;
	    }
	    if (++n == BUILD_MENU_THRESHOLD)
		n = 0;
	}
	else
	    menu = build_menu;
	    
    	if (!strcmp(targ, "-"))
	    ui_add_separator(menu);
	else
	    ui_add_button_2(menu, targ, FALSE, 0, build_cb, targ,
	    			GR_NOTRUNNING);
    }
}

static gboolean
is_standard_target(const char *targ)
{
    const char **tp;
    
    for (tp = standard_targets ; *tp ; tp++)
    {
	if (!strcmp(*tp, targ))
	    return TRUE;
    }
    return FALSE;
}

typedef struct
{
    Task task;
    estring targets;
} ListTargetsTask;

static void
list_targets_reap(Task *task)
{
    ListTargetsTask *ltt = (ListTargetsTask *)task;
    char *t, *buf;
    GList *std = 0;

    /*
     * First, remove list of available targets from previous run
     */
    while (available_targets != 0)
    {
	g_free((char *)available_targets->data);
	available_targets = g_list_remove_link(available_targets, available_targets);
    }
    ui_delete_menu_items(build_menu);
    construct_build_menu_basic_items();
     
    /* 
     * Parse the output of the program into whitespace-separated
     * strings which are targets. Build two lists, std (all
     * the found targets which are also in `standard_targets')
     * and available_targets (all others).
     */
    buf = ltt->targets.data;
    if (buf != 0)
    {
	while ((t = strtok(buf, " \t\r\n")) != 0)
	{
	    {
		t = g_strdup(t);
		if (is_standard_target(t))
		{
#if DEBUG
    	    	    fprintf(stderr, "reap_list adding standard target \"%s\"\n", t);
#endif
    	    	    std = g_list_append(std, t);
		}
		else
		{
#if DEBUG
    	    	    fprintf(stderr, "reap_list adding target \"%s\"\n", t);
#endif
    	    	    available_targets = g_list_append(available_targets, t);
		}
    	    }
	    buf = 0;
	}
    }
    estring_free(&ltt->targets);
    
    /*
     * Build two parts of the menu from the two lists. This
     * technique ensures the GNU standard targets like `all'
     * and `install' will appear early in the menu and
     * will always be visible regardless of its size.
     */
    append_build_menu_items(std);
    if (std != 0 && available_targets != 0)
	ui_add_separator(build_menu);
    append_build_menu_items(available_targets);

    /*
     * Now prepend `std' to `available_targets', which is now a
     * list of all the found targets, standard or not.
     */
    available_targets = g_list_concat(std, available_targets);

    grey_menu_items();
}

static void
list_targets_input(Task *task, int len, const char *buf)
{
    ListTargetsTask *ltt = (ListTargetsTask *)task;

    estring_append_chars(&ltt->targets, buf, len);
}

static void
list_targets_destroy(Task *task)
{
    ListTargetsTask *ltt = (ListTargetsTask *)task;
    estring_free(&ltt->targets);
}

static TaskOps list_targets_ops =
{
0,  	    	    	    	/* start */
list_targets_input,	    	/* input */
list_targets_reap, 	    	/* reap */
list_targets_destroy	    	/* destroy */
};

static Task *
list_targets_task(void)
{
    ListTargetsTask *ltt = g_new(ListTargetsTask, 1);

    estring_init(&ltt->targets);
    return task_create(
    	(Task *)ltt,
	expand_prog(prefs.prog_list_targets, 0, 0, 0),
	prefs.var_environment,
	&list_targets_ops);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
list_targets(void)
{
    if (prefs.enable_make_makefile)
	task_enqueue(make_makefile_task());
    task_enqueue(list_targets_task());
    task_start();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static TaskOps editor_ops =
{
0,  	    	    	/* start */
0,	    	  	/* input */
0, 	    	  	/* reap */
0	    	    	/* destroy */
};

static Task *
editor_task(const char *file, int line)
{
    return task_create(
    	(Task *)g_new(Task, 1),
	expand_prog(prefs.prog_edit_source, file, line, 0),
	0/*env*/,
	&editor_ops);
}


static void
start_edit(LogRec *lr)
{
    if (lr->res.file == 0 || lr->res.file[0] == '\0')
    	return;
	
    message(_("Editing %s (line %d)"), lr->res.file, lr->res.line);
    task_spawn(editor_task(lr->res.file, lr->res.line));	
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
handle_line(const char *line)
{
    LogRec *lr;

#if DEBUG > 38
    fprintf(stderr, "handle_line(): \"%s\"\n", line);
#endif

    lr = log_add_line(line);
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
}


/*
 * Call when input has been received from a child process.
 * Handles splitting the input into lines, and calls
 * handle_line() to do what needs to be done per-line.
 * Should be called once with `len'=0 when the child
 * process has been reaped.
 */
 
static void
handle_input(int len, const char *buf)
{
    static estring leftover = ESTRING_STATIC_INIT;
    
    if (len == 0)
    {
    	/* end of input */
	/*
	 * Handle case where last line of child process'
	 * output has no terminating '\n'. Beware - 
	 * this last line may contain an error or warning,
	 * which affects log_num_{errors,warnings}().
	 */
	if (leftover.length > 0)
	    handle_line(leftover.data);
	/*
	 * Free'ing and reinitialising seems a bit extreme,
	 * but it allows the program to give back to malloc()
	 * a large line buffer allocated for a single unusual
	 * very long line.
	 */
	estring_free(&leftover);
	estring_init(&leftover);
	return;
    }
    
    while (len > 0 && *buf)
    {
	char *p = strchr(buf, '\n');
	if (p == 0)
	{
    	    /* only a part of a line left - append to leftover */
	    estring_append_string(&leftover, buf);
	    return;
	}
    	/* got an end-of-line - isolate the line & feed it to handle_line() */
	*p = '\0';
	estring_append_string(&leftover, buf);

    	handle_line(leftover.data);
	
	estring_truncate(&leftover);
	len -= (p - buf);
	buf = ++p;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    Task task;
    const char *target;
} MakeTask;

static void
make_start(Task *task)
{
    MakeTask *mt = (MakeTask *)task;
    
    message(_("Making %s"), mt->target);

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

    set_last_target(mt->target);
    interrupted = FALSE;
    log_start_build(task->command);
}

static void
make_input(Task *task, int len, const char *buf)
{
    handle_input(len, buf);
}

static void
make_reap(Task *task)
{
    MakeTask *mt = (MakeTask *)task;
    char *err_str = 0, *warn_str = 0, *int_str = 0;
    
    handle_input(0, 0);     /* deal with possibly unterminated last line */

    if (log_num_errors() > 0)
	err_str = g_strdup_printf(_(", %d errors"), log_num_errors());

    if (log_num_warnings() > 0)
	warn_str = g_strdup_printf(_(", %d warnings"), log_num_warnings());

    if (interrupted)
	int_str = _(" (interrupted)");

    message(_("Finished making %s%s%s%s"),
	mt->target,
	safe_str(err_str),
	safe_str(warn_str),
	safe_str(int_str));

    if (err_str != 0)
	g_free(err_str);
    if (warn_str != 0)
	g_free(warn_str);

    log_end_build(mt->target);
}

static TaskOps make_ops =
{
make_start,        	/* start */
make_input,	    	/* input */
make_reap, 	    	/* reap */
0	    	    	/* destroy */
};

static Task *
make_task(const char *target)
{
    MakeTask *mt = g_new(MakeTask, 1);

    mt->target = target;
    return task_create(
    	(Task *)mt,
	expand_prog(prefs.prog_make, 0, 0, target),
	prefs.var_environment,
	&make_ops);
}


void
build_start(const char *target)
{
    first_error = TRUE;
    if (prefs.enable_make_makefile)
	task_enqueue(make_makefile_task());
    task_enqueue(make_task(target));
    task_start();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
toplevel_resize_cb(GtkWidget *w, GdkEvent *ev, gpointer data)
{
    static int curr_width = -1, curr_height = -1;
    int width, height;
    
    width = ev->configure.width;
    height = ev->configure.height;
    
    /*
     * This weeds out the ConfigureNotify events we get
     * from the X server when the window is moved.
     */
    if (curr_width >= 0 &&
        (curr_width != width || curr_height != height))
    {
#if DEBUG
	fprintf(stderr, "toplevel_resize_cb: Saving new size\n");
#endif
	preferences_resize(w->allocation.width, w->allocation.height);
    }
    
    curr_width = width;
    curr_height = height;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_exit_cb(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_open_file_func(const char *filename)
{
    message("Opening log file %s", filename);
    message_flush();
    log_open(filename);
}

static void
file_open_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(
	    toplevel,
	    _("Maketool: Open Log File"),
	    file_open_file_func,
	    "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_save_file_func(const char *filename)
{
    message("Saving log file %s", filename);
    message_flush();
    log_save(filename);
}

static void
file_save_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(
	    toplevel,
	    _("Maketool: Save Log File"),
	    file_save_file_func,
	    "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static void
dump_dir_history(const char *str)
{
    GList *list;
    
    fprintf(stderr, "%s dir_history = {\n", str);
    for (list = dir_history ; list != 0 ; list = list->next)
	fprintf(stderr, "   %s\n", (char*)list->data);
    fprintf(stderr, "}\n");
}
#endif


static void
construct_dir_previous_menu(void)
{
    GList *list;
    int i;
    char accelbuf[32];
    
    /* first delete previously existing items */
    ui_delete_menu_items(dir_previous_menu);

    /* now construct a new menu */    
    for (list = dir_history, i = 0 ; list != 0 ; list = list->next, i++)
    {
    	char *dir = (char *)list->data;
	
	if (i < 9)
	    g_snprintf(accelbuf, sizeof(accelbuf), "<Ctrl>%c", (i+'1'));
	ui_add_button_2(dir_previous_menu, dir, FALSE, (i<9 ? accelbuf : 0),
	    	dir_previous_cb, dir, GR_NOTRUNNING);
    }
    
    if (dir_history == 0)
	ui_add_button(dir_previous_menu, _("No previous directories"), 0, 0, 0, GR_NEVER);
}


static gboolean
change_directory(const char *dir)
{
    char *olddir;
    
    olddir = g_get_current_dir();
#if DEBUG > 5
    fprintf(stderr, "Changing dir from %s to: %s\n", olddir, dir);
#endif
    
    if (chdir(dir) < 0)
    {
    	g_free(olddir);
    	return FALSE;
    }

    /*
     * Update directory history
     */
    if (g_list_find_str(dir_history, olddir) != 0)
    {
    	/* already in list */
	g_free(olddir);
    }
    else
    {
	/* not already in history list, prepend it */
	dir_history = g_list_prepend(dir_history, olddir);

	/* trim list to fit */
	while (g_list_length(dir_history) > MAX_DIR_HISTORY)
	{
	    GList *last = g_list_last(dir_history);
	    g_free((char *)last->data);
	    dir_history = g_list_remove_link(dir_history, last);
	}

    	/* update Previous Directory menu */
	if (dir_previous_menu != 0)
    	    construct_dir_previous_menu();
    }
	
	
    /* update UI for new dir */
    
    if (toplevel != 0)
	set_main_title();
	
    if (messageent != 0)
    {
    	char *curr = g_get_current_dir();
	message("Directory %s", curr);
	g_free(curr);
    }
	
    if (build_menu != 0)
    	list_targets();
    
    return TRUE;
}

static void
dir_previous_cb(GtkWidget *w, gpointer data)
{
    change_directory((char*)data);
}

static void
file_change_dir_func(const char *filename)
{
    change_directory(filename);
}

static void
file_change_dir_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    char *currdir, *fakefile;
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(
	    toplevel,
	    _("Maketool: Change Directory"),
	    file_change_dir_func,
	    ".");
	    
    /*
     * Tel the filesel window the current directory.
     * Filesel window needs the trailing / so it doesn't
     * try to interpret the dirname as a filename.
     */
    currdir = g_get_current_dir();
    fakefile = g_strdup_printf("%s/", currdir);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), fakefile);
    g_free(currdir);
    g_free(fakefile);
    
    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_edit_makefile_cb(GtkWidget *w, gpointer data)
{
    const char *makefile = 0;
    
    /*
     * Have to do a little dance to figure out what
     * filename gmake will use for the makefile.
     * First check explicit -f option set in preferences.
     */
    if (prefs.makefile != 0)
    	makefile = prefs.makefile;
    else
    {
    	/*
	 * Now we have to do it the hard way, attempting to
	 * reproduce the filename search behaviour of gmake.
	 */
	static const char *search_filenames[] = {
	"GNUmakefile", "makefile", "Makefile", 0
	};
	const char **fn;
	
	for (fn = search_filenames ; *fn != 0 ; fn++)
	{
	    if (file_exists(*fn))
	    {
	    	makefile = *fn;
		break;
	    }
	}
	
	if (makefile == 0 || !strcmp(makefile, "Makefile"))
	{
	    /* attempt to deal with autoconf, automake, and imake */
	    static const char *meta_makefiles[] = {
	    "Makefile.am", "Makefile.in", "Imakefile", 0
	    };
	    for (fn = meta_makefiles ; *fn != 0 ; fn++)
	    {
		if (file_exists(*fn))
		{
	    	    makefile = *fn;
		    break;
		}
	    }
	}
    }
    
    if (makefile == 0)
    {
    	message(_("No makefile appears to be present in this directory"));
	return;
    }
     
    message(_("Editing %s"), makefile, 1);
    task_spawn(editor_task(makefile, 1));	
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_again_cb(GtkWidget *w, gpointer data)
{
    if (last_target != 0 && !task_is_running())
    	build_start(last_target);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_stop_cb(GtkWidget *w, gpointer data)
{
    if (task_is_running())
    {
    	interrupted = TRUE;
	task_kill_current();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_dryrun_cb(GtkWidget *w, gpointer data)
{
    preferences_set_dryrun(GTK_CHECK_MENU_ITEM(w)->active);
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
edit_error_cb(GtkWidget *w, gpointer data)
{
    LogRec *lr = log_selected();

    if (lr != 0)    
	start_edit(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
edit_next_error_cb(GtkWidget *w, gpointer data)
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
edit_copy_cb(GtkWidget *w, gpointer data)
{
    LogRec *lr = log_selected();

    assert(lr != 0);

    if (clipboard_text != 0)
    	g_free(clipboard_text);
    clipboard_text = g_strdup_printf("%s\n", log_get_text(lr));
#if DEBUG
    fprintf(stderr, "Copying `%s'\n", clipboard_text);
#endif

    gtk_selection_owner_set(GTK_WIDGET(clipboard_widget), clipboard_atom,
    	    GDK_CURRENT_TIME);
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

/*
 * Called when a ctree row is selected or unselected
 */
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
construct_build_menu_basic_items(void)
{
    again_menu_item = ui_add_button(build_menu, _("_Again"), "<Ctrl>A", build_again_cb, 0, GR_AGAIN);
    ui_add_button(build_menu, _("_Stop"), 0, build_stop_cb, 0, GR_RUNNING);
    ui_add_toggle(build_menu, _("_Dryrun Only"), "<Ctrl>D", build_dryrun_cb, 0,
    	0, prefs.dryrun);
    ui_add_separator(build_menu);
}


static void
ui_create_menus(GtkWidget *menubar)
{
    GtkWidget *menu;
    GtkAccelGroup *ag;
    
    ag = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(toplevel), ag);
    ui_set_accel_group(ag);

    
    menu = ui_add_menu(menubar, _("_File"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("_Open Log..."), "<Ctrl>O", file_open_cb, 0, GR_NONE);
    ui_add_button(menu, _("_Save Log..."), "<Ctrl>S", file_save_cb, 0, GR_NOTEMPTY);
    ui_add_separator(menu);
    ui_add_button(menu, _("_Change Directory..."), 0, file_change_dir_cb, 0, GR_NOTRUNNING);
    dir_previous_menu = ui_add_submenu(menu, TRUE, _("_Previous Directories"));
    ui_add_tearoff(dir_previous_menu);
    construct_dir_previous_menu();
    ui_add_separator(menu);
    ui_add_button(menu, _("_Edit Makefile..."), 0, file_edit_makefile_cb, 0, GR_NOTRUNNING);
    ui_add_separator(menu);
    ui_add_button(menu, _("_Print..."), "<Ctrl>P", file_print_cb, 0, GR_NOTEMPTY);
    ui_add_separator(menu);
    ui_add_button(menu, _("E_xit"), "<Ctrl>X", file_exit_cb, 0, GR_NONE);
    
    menu = ui_add_menu(menubar, _("_Edit"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("Edit _Error"), 0, edit_error_cb, 0, GR_EDITABLE);
    ui_add_button(menu, _("Edit _Next Error"), "<Ctrl>E", edit_next_error_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    ui_add_button(menu, _("Edit _Prev Error"), 0, edit_next_error_cb, GINT_TO_POINTER(FALSE), GR_NOTEMPTY);
    ui_add_separator(menu);
    ui_add_button(menu, _("_Copy"), "<Ctrl>C", edit_copy_cb, 0, GR_SELECTED);
    ui_add_separator(menu);
    ui_add_button(menu, _("_Find"), "<Ctrl>F", edit_find_cb, 0, GR_NOTEMPTY);
    ui_add_button(menu, _("Find _Again"), "<Ctrl>G", edit_find_again_cb, 0, GR_FIND_AGAIN);
    ui_add_separator(menu);
    ui_add_button(menu, _("Pre_ferences..."), 0, edit_preferences_cb, 0, GR_NONE);

    build_menu = ui_add_menu(menubar, _("_Build"));
    ui_add_tearoff(build_menu);
    construct_build_menu_basic_items();
    
    menu = ui_add_menu(menubar, _("_View"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("_Clear Log"), 0, view_clear_cb, 0, GR_CLEAR_LOG);
    ui_add_button(menu, _("C_ollapse All"), 0, view_collapse_all_cb, 0, GR_NOTEMPTY);
    ui_add_separator(menu);
    ui_add_toggle(menu, _("_Toolbar"), 0, view_widget_cb, (gpointer)&toolbar_hb, 0, TRUE);
    ui_add_toggle(menu, _("_Statusbar"), 0, view_widget_cb, (gpointer)&messagebox, 0, TRUE);
    ui_add_separator(menu);
    ui_add_toggle(menu, _("_Errors"), 0, view_flags_cb, GINT_TO_POINTER(LF_SHOW_ERRORS),
    	0, log_get_flags() & LF_SHOW_ERRORS);
    ui_add_toggle(menu, _("_Warnings"), 0, view_flags_cb, GINT_TO_POINTER(LF_SHOW_WARNINGS),
    	0, log_get_flags() & LF_SHOW_WARNINGS);
    ui_add_toggle(menu, _("_Information"), 0, view_flags_cb, GINT_TO_POINTER(LF_SHOW_INFO),
    	0, log_get_flags() & LF_SHOW_INFO);
    ui_add_toggle(menu, _("_Summary"), 0, view_flags_cb, GINT_TO_POINTER(LF_SUMMARISE),
    	0, log_get_flags() & LF_SUMMARISE);
    ui_add_toggle(menu, _("_Indent Directories"), 0, view_flags_cb, GINT_TO_POINTER(LF_INDENT_DIRS),
    	0, log_get_flags() & LF_INDENT_DIRS);
    
    menu = ui_add_menu_right(menubar, _("_Help"));
    ui_add_tearoff(menu);
    ui_add_button(menu, _("_About Maketool..."), 0, help_about_cb, 0, GR_NONE);
    ui_add_button(menu, _("About _make..."), 0, help_about_make_cb, 0, GR_NONE);
#if 0
    ui_add_separator(menu);
    ui_add_button(menu, _("_Help on..."), 0, unimplemented, 0, GR_NONE);
    ui_add_separator(menu);
    ui_add_button(menu, _("_Tutorial"), 0, unimplemented, 0, GR_NONE);
    ui_add_button(menu, _("_Reference Index"), 0, unimplemented, 0, GR_NONE);
    ui_add_button(menu, _("_Home Page"), 0, unimplemented, 0, GR_NONE);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "all.xpm"
#include "again.xpm"
#include "stop.xpm"
#include "clean.xpm"
#include "clear.xpm"
#include "next.xpm"
#include "print.xpm"

static void
ui_create_tools(GtkWidget *toolbar)
{
    char *tooltip;
    
    again_tool_item = ui_tool_create(toolbar, _("Again"), _("Build last target again"),
    	again_xpm, build_again_cb, 0, GR_AGAIN);

    ui_tool_create(toolbar, _("Stop"), _("Stop current build"),
    	stop_xpm, build_stop_cb, 0, GR_RUNNING);
    
    ui_tool_add_space(toolbar);

    tooltip = g_strdup_printf(_("Build `%s'"), "all");
    ui_tool_create(toolbar, "all", tooltip,
    	all_xpm, build_cb, "all", GR_ALL);
    g_free(tooltip);
    
    tooltip = g_strdup_printf(_("Build `%s'"), "clean");
    ui_tool_create(toolbar, "clean", tooltip,
    	clean_xpm, build_cb, "clean", GR_CLEAN);
    g_free(tooltip);
    
    ui_tool_add_space(toolbar);
    
    ui_tool_create(toolbar, _("Clear"), _("Clear log"),
    	clear_xpm, view_clear_cb, 0, GR_CLEAR_LOG);
    ui_tool_create(toolbar, _("Next"), _("Edit next error or warning"),
    	next_xpm, edit_next_error_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    
    ui_tool_add_space(toolbar);
    
    ui_tool_create(toolbar, _("Print"), _("Print log"),
    	print_xpm, file_print_cb, 0, GR_NOTEMPTY);
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

static void
clipboard_get(GtkWidget *w, GtkSelectionData *seldata, guint info, guint time)
{
#if DEBUG
    fprintf(stderr, "clipboard_get(info=%d)\n", info);
#endif

    switch (info)
    {
    case SEL_TARGET_STRING:
    	gtk_selection_data_set(seldata,
		    GDK_SELECTION_TYPE_STRING, 8,
		    (guchar*)clipboard_text, strlen(clipboard_text));
    	break;
    case SEL_TARGET_TEXT:
    case SEL_TARGET_COMPOUND_TEXT:
	{
	    guchar *ctext;
	    GdkAtom encoding;
	    gint format;
	    gint new_length;

	    gdk_string_to_compound_text((char*)clipboard_text, &encoding, &format, &ctext, &new_length);
	    gtk_selection_data_set(seldata, encoding, format, ctext, new_length);
	    gdk_free_compound_text(ctext);
	}
    	break;
    default:
    	fprintf(stderr, "clipboard_get: unknown info %d\n", info);
	return;
    }
}


static void
clipboard_init(GtkWidget *w)
{
    static const GtkTargetEntry targets[] = {
    { "STRING", 0, SEL_TARGET_STRING },
    { "TEXT",   0, SEL_TARGET_TEXT }, 
    { "COMPOUND_TEXT", 0, SEL_TARGET_COMPOUND_TEXT }
    };

    clipboard_widget = w;
    clipboard_atom = gdk_atom_intern("CLIPBOARD", FALSE);
    gtk_selection_add_targets(GTK_WIDGET(w), clipboard_atom,
			 targets, ARRAYLEN(targets));

    gtk_signal_connect(GTK_OBJECT(w), "selection_get", 
    	GTK_SIGNAL_FUNC(clipboard_get), NULL);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Set the title of the main window to
 * reflect maketool's current directory.
 */
 
static void
set_main_title(void)
{
    char *pwd = g_get_current_dir();
    char *base;
    char *title;
    const char *title_prefix = _("Maketool");

    if (pwd == 0)
    	base = 0;
    else
    {    
	base = strrchr(pwd, '/');
	if (base != 0 && base != pwd)   /* handle pwd=="/" */
	    base++;
    }
    /*
     * At this point, `base' is a readable representation
     * of the basename of the current working directory, or 0.
     */
    if (base == 0)
	title = g_strdup_printf("%s %s", title_prefix, VERSION);
    else
	title = g_strdup_printf("%s %s: %s", title_prefix, VERSION, base);
    gtk_window_set_title(GTK_WINDOW(toplevel), title);
    g_free(title);
    if (pwd != 0)
	g_free(pwd);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "maketool.xpm"

static void
ui_create(void)
{
    GtkWidget *table;
    GtkWidget *menubar, *logwin;
    GtkWidget *handlebox;
    GtkTooltips *tooltips;
    GtkWidget *sw;
    GdkPixmap *iconpm;
    GdkBitmap *iconmask = 0;
    static char *titles[1] = { "Log" };
    
    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    set_main_title();
    gtk_window_set_default_size(GTK_WINDOW(toplevel),
    	prefs.win_width, prefs.win_height);
    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
    	GTK_SIGNAL_FUNC(file_exit_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(toplevel), "configure_event", 
    	GTK_SIGNAL_FUNC(toplevel_resize_cb), NULL);
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
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    
    handlebox = gtk_handle_box_new();
    gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(handlebox), GTK_SHADOW_NONE);
    gtk_table_attach(GTK_TABLE(table), handlebox, 0, 1, 0, 1,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, 0,
	0, 0);
    gtk_widget_show(handlebox);

    menubar = gtk_menu_bar_new();
    gtk_container_add(GTK_CONTAINER(handlebox), menubar);
    gtk_widget_show(menubar);
    ui_create_menus(menubar);
        
    handlebox = gtk_handle_box_new();
    gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(handlebox), GTK_SHADOW_NONE);
    /* gtk_container_border_width(GTK_CONTAINER(handlebox), SPACING); */
    gtk_table_attach(GTK_TABLE(table), handlebox, 0, 1, 1, 2,
    	GTK_FILL|GTK_EXPAND|GTK_SHRINK, 0,
	0, 0);
    gtk_widget_show(handlebox);
    toolbar_hb = handlebox;

    toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
    gtk_toolbar_set_space_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_SPACE_LINE);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_container_add(GTK_CONTAINER(handlebox), toolbar);
    gtk_widget_show(toolbar);
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
    gtk_signal_connect(GTK_OBJECT(logwin), "tree-unselect-row", 
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
    clipboard_init(logwin);
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

    list_targets();

    gtk_widget_show(GTK_WIDGET(table));
    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_string[] = "\
Usage: maketool [options] [target] [var=value] ...\n\
Options:\n\
  -C DIRECTORY, --directory=DIRECTORY\n\
                              Change to DIRECTORY before doing anything.\n\
  -f FILE, --file=FILE, --makefile=FILE\n\
                              Read FILE as a makefile.\n\
  -h, --help                  Print this message and exit.\n\
  -j [N], --jobs[=N]          Allow N jobs at once; infinite jobs with no arg.\n\
  -k, --keep-going            Keep going when some targets can't be made.\n\
  -l [N], --load-average[=N], --max-load[=N]\n\
                              Don't start multiple jobs unless load is below N.\n\
  -S, --no-keep-going, --stop\n\
                              Turns off -k.\n\
  -v, --version               Print the version number of maketool and exit.\n\
";

static const char version_string[] = "\
Maketool " VERSION "\n\
(c) 1999-2000 Greg Banks <gnb@alphalink.com.au>\n\
Maketool comes with ABSOLUTELY NO WARRANTY.\n\
You may redistribute copies of this software\n\
under the terms of the GNU General Public\n\
Licence; see the file COPYING.\n\
";


static void
usage(int code)
{
    fputs(usage_string, stderr);
    exit(code);
}

static void
set_makefile(const char *mf)
{
    if (prefs.makefile != 0)
    	g_free(prefs.makefile);
    prefs.makefile = g_strdup(mf);
}

static void
set_directory(const char *dir)
{
    if (!change_directory(dir))
    {
    	perror(dir);
    	exit(1);
    }
}

#ifdef ARGSTEST
char *original_dir = 0;
static char *
relative_dir(void)
{
    char *curr_dir = g_get_current_dir();
    int i, j = 0;
    estring rd;
    
    for (i=0 ;
         curr_dir[i] && original_dir[i] && curr_dir[i] == original_dir[i] ;
	 i++)
    	if (curr_dir[i] == '/')
	    j = i;
    if (!original_dir[i])
    	j = i;

    estring_init(&rd);
    if (j > 0)
    	estring_append_string(&rd, "...");
    estring_append_string(&rd, curr_dir+j);
    return rd.data;
}
#endif

static void
parse_args(int argc, char **argv)
{
    int i;
    
#ifdef ARGSTEST
    original_dir = g_get_current_dir();
#endif

    argv0 = argv[0];
    cmd_targets = g_new(char*, argc);
    cmd_targets[cmd_num_targets = 0] = 0;
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "-C"))
	    {
	    	if (i == argc-1)
		    usage(1);
	    	set_directory(argv[++i]);
	    }
	    else if (!strncmp(argv[i], "--directory=", 12))
	    {
	    	set_directory(argv[i]+12);
	    }
	    else if (!strcmp(argv[i], "-f"))
	    {
	    	if (i == argc-1)
		    usage(1);
	    	set_makefile(argv[++i]);
	    }
	    else if (!strncmp(argv[i], "--file=", 7))
	    {
	    	set_makefile(argv[i]+7);
	    }
	    else if (!strncmp(argv[i], "--makefile=", 11))
	    {
	    	set_makefile(argv[i]+11);
	    }
	    else if (!strcmp(argv[i], "-h") ||
	             !strcmp(argv[i], "--help"))
	    {
	    	usage(0);
	    }
	    else if (!strcmp(argv[i], "-j"))
	    {
	    	prefs.run_how = RUN_PARALLEL_PROC;
		prefs.run_processes = (i < argc-1 && isdigit(argv[i+1][0]) ? atoi(argv[++i]) : 0);
	    }
	    else if (!strncmp(argv[i], "--jobs", 6))
	    {
	    	prefs.run_how = RUN_PARALLEL_PROC;
		prefs.run_processes = (argv[i][6] == '=' ? atoi(argv[i]+7) : 0);
	    }
	    else if (!strcmp(argv[i], "-k") ||
	             !strcmp(argv[i], "--keep-going"))
	    {
	    	prefs.ignore_failures = TRUE;
	    }
	    else if (!strcmp(argv[i], "-l"))
	    {
	    	prefs.run_how = RUN_PARALLEL_LOAD;
		prefs.run_load = (i < argc-1 && isdigit(argv[i+1][0]) ? (int)(atof(argv[++i])*10.0) : 0);
	    }
	    else if (!strncmp(argv[i], "--load-average", 14))
	    {
	    	prefs.run_how = RUN_PARALLEL_LOAD;
		prefs.run_load = (argv[i][14] == '=' ? (int)(atof(argv[i]+15)*10.0) : 0);
	    }
	    else if (!strncmp(argv[i], "--max-load", 6))
	    {
	    	prefs.run_how = RUN_PARALLEL_LOAD;
		prefs.run_load = (argv[i][10] == '=' ? (int)(atof(argv[i]+11)*10.0) : 0);
	    }
	    else if (!strcmp(argv[i], "-S") ||
	             !strcmp(argv[i], "--no-keep-going") ||
	             !strcmp(argv[i], "--stop"))
	    {
	    	prefs.ignore_failures = FALSE;
	    }
	    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
	    {
	    	fputs(version_string, stdout);
	    	exit(0);
	    }
	    /* TODO: add -n --dryrun flag */
	    else
	    {
		usage(1);
	    }
	}
	else
	{
	    if (strchr(argv[i], '=') != 0)
	    {
	    	/* variable override */
		char *buf = g_strdup(argv[i]);
		char *value = strchr(buf, '=');
		*value++ = '\0';
		preferences_add_variable(buf, value, VAR_MAKE);
		g_free(buf);
	    }
	    else
	    {
	    	/* target */
		cmd_targets[cmd_num_targets++] = argv[i];
	    }
	}
    }
    

#ifdef ARGSTEST
    {
	static const char *run_how_strings[] = {
	    "RUN_SERIES", "RUN_PARALLEL_PROC", "RUN_PARALLEL_LOAD"};
	static const char *bool_strings[] = {
	    "FALSE", "TRUE"};
    	int i;
	    
	printf("makefile = %s\n", prefs.makefile);
	printf("directory = %s\n", relative_dir());
	printf("targets = {");
	for (i=0 ; i<cmd_num_targets ; i++)
	    printf(" %s", cmd_targets[i]);
	printf(" }\n");
	printf("ignore_failures = %s\n", bool_strings[prefs.ignore_failures]);
	printf("run_how = %s\n", run_how_strings[prefs.run_how]);
	printf("run_processes = %d\n", prefs.run_processes);
	printf("run_load = %d\n", prefs.run_load);
	exit(0);
    }
#endif
}

static void
enqueue_cmd_targets(void)
{
    int i;

    if (cmd_num_targets == 0)
    	return;

    /* Don't enqueue make_makefiles_task() 'cos it's just
     * been done when we called list_targets()
     */	
    for (i=0 ; i<cmd_num_targets ; i++)
	task_enqueue(make_task(cmd_targets[i]));
    task_start();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    
    gtk_init(&argc, &argv);    
    preferences_load();
    parse_args(argc, argv);
    task_init(work_started, work_ended);
    ui_create();
    enqueue_cmd_targets();
    gtk_main();
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
