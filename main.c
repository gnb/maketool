#define DEFINE_GLOBALS
#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>
#include "maketool.h"
#include "spawn.h"
#include "ui.h"
#include "log.h"
#include "util.h"

CVSID("$Id: main.c,v 1.16 1999-05-25 12:48:02 gnb Exp $");

typedef enum
{
    GR_NONE=-1,
    
    GR_NOTEMPTY=0,
    GR_NOTRUNNING,
    GR_RUNNING,
    GR_EDITABLE,
    GR_AGAIN,

    NUM_SETS
} Groups;

char		*targets;		/* targets on commandline */
const char	*lastTarget = 0;	/* last target built, for `again' */
GList		*availableTargets = 0;	/* all possible targets, for menu */
GtkWidget	*buildMenu;
GtkWidget	*toolbar, *messagebox;
GtkWidget	*messageent;
pid_t		currentPid = -1;
gboolean	interrupted = FALSE;
gboolean	firstError = FALSE;

#define ANIM_MAX 8
GdkPixmap	*animPixmaps[ANIM_MAX+1];
GdkBitmap	*animMasks[ANIM_MAX+1];
GtkWidget	*anim;


#define PASTE3(x,y,z) x##y##z

static void build_cb(GtkWidget *w, gpointer data);


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
message(const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    
    gtk_entry_set_text(GTK_ENTRY(messageent), buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
grey_menu_items(void)
{
    gboolean running = (currentPid > 0);
    gboolean empty = logIsEmpty();
    LogRec *sel = logSelected();
    gboolean editable = (sel != 0 && sel->res.file != 0);
    gboolean again = (lastTarget != 0 && !running);

    uiGroupSetSensitive(GR_NOTRUNNING, !running);
    uiGroupSetSensitive(GR_RUNNING, running);
    uiGroupSetSensitive(GR_NOTEMPTY, !empty);
    uiGroupSetSensitive(GR_EDITABLE, editable);
    uiGroupSetSensitive(GR_AGAIN, again);
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
    	animPixmaps[anim_current], animMasks[anim_current]);
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
    	animPixmaps[0], animMasks[0]);
}

static void
anim_start(void)
{
    if (anim_timer < 0)
	anim_timer = gtk_timeout_add(200/* millisec */, anim_advance, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reapList(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    estring *targs = (estring *)user_data;
    char *t, *buf;

    if (!(WIFEXITED(status) || WIFSIGNALED(status)))
    	return;
	
    fprintf(stderr, "reaped list target\n");
    buf = targs->data;
    while ((t = strtok(buf, " \t\r\n")) != 0)
    {
    	if (!strcmp(t, "-"))
	{
	    uiAddSeparator(buildMenu);	
	}
	else
	{
    	   t = strdup(t);
    	   availableTargets = g_list_append(availableTargets, t);
	   uiAddButton(buildMenu, t, build_cb, t, GR_NOTRUNNING);
    	}
	buf = 0;
    }
    estring_free(targs);
    grey_menu_items();
}

static void
inputList(int len, const char *buf, gpointer data)
{
    estring *targs = (estring *)data;
    
    estring_append_chars(targs, buf, len);
}

static void
listTargets(void)
{
    char *prog;
    pid_t pid;
    static estring targs;
    
    estring_init(&targs);
    
    prog = expand_prog(prefs.prog_list_targets, 0, 0, 0);
    
    pid = spawn_with_output(prog, reapList, inputList, (gpointer)&targs,
    	prefs.var_environment);
    if (pid > 0)
    {
    	/* TODO: remove old menu items */
    }
    free(prog);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
startEdit(LogRec *lr)
{
    char *prog;
    
    if (lr->res.file == 0 || lr->res.file[0] == '\0')
    	return;
	
    prog = expand_prog(prefs.prog_edit_source, lr->res.file, lr->res.line, 0);
    
    if (spawn_simple(prog, 0, 0) > 0)
	message(_("Editing %s (line %d)"), lr->res.file, lr->res.line);
	
    free(prog);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reapMake(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    char *target = (char *)user_data;
    
    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
    	char errStr[256];
    	char warnStr[256];
	char intStr[256];
	
	if (logNumErrors() > 0)
	    sprintf(errStr, _(", %d errors"), logNumErrors());
	else
	    errStr[0] = '\0';
	    
	if (logNumWarnings() > 0)
	    sprintf(warnStr, _(", %d warnings"), logNumWarnings());
	else
	    warnStr[0] = '\0';

	if (interrupted)
	    strcpy(intStr, _(" (interrupted)"));
	else
	    intStr[0] = '\0';
	    
	message(_("Finished making %s%s%s%s"), target, errStr, warnStr, intStr);
	
	currentPid = -1;
	logEndBuild(target);
	anim_stop();
	grey_menu_items();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
handleInput(int len, const char *buf, gpointer data)
{
    static char linebuf[1024];
    static int nleftover = 0;
    LogRec *lr;
    
#if DEBUG   
    fprintf(stderr, "handleData(): \"");
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
    	/* got an end-of-line - isolate the line & feed it to handleLine() */
	*p = '\0';
	strncpy(&linebuf[nleftover], buf, sizeof(linebuf)-nleftover);

	lr = logAddLine(linebuf);
	if (prefs.edit_first_error && firstError)
	{
	    if ((lr->res.code == FR_WARNING && prefs.edit_warnings) ||
	         lr->res.code == FR_ERROR)
	    {
	        firstError = FALSE;
	    	logSetSelected(lr);
		startEdit(lr);
	    }
	}
	
	nleftover = 0;
	len -= (p - buf);
	buf = ++p;
    }
}

void
buildStart(const char *target)
{
    char *prog;
    
    prog = expand_prog(prefs.prog_make, 0, 0, target);
    
    currentPid = spawn_with_output(prog, reapMake, handleInput, (gpointer)target,
    	prefs.var_environment);
    if (currentPid > 0)
    {
	message(_("Making %s"), target);
	
	switch (prefs.start_action)
	{
	case START_NOTHING:
	    break;
	case START_CLEAR:
	    logClear();
	    break;
	case START_COLLAPSE:
	    logCollapseAll();
	    break;
	}
	
	lastTarget = target;
	interrupted = FALSE;
	firstError = TRUE;
	logStartBuild(prog);
	anim_start();
	grey_menu_items();
    }
    free(prog);
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
    	filesel = uiCreateFileSel(_("Open Log File"), logOpen, "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_save_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = uiCreateFileSel(_("Save Log File"), logSave, "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_again_cb(GtkWidget *w, gpointer data)
{
    if (lastTarget != 0 && currentPid < 0)
    	buildStart(lastTarget);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_stop_cb(GtkWidget *w, gpointer data)
{
    if (currentPid > 0)
    {
    	interrupted = TRUE;
    	kill(currentPid, SIGKILL);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_cb(GtkWidget *w, gpointer data)
{
    buildStart((char *)data);    
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
	logSetFlags(logGetFlags() | flags);
    else
	logSetFlags(logGetFlags() & ~flags);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_clear_cb(GtkWidget *w, gpointer data)
{
    logClear();
}

static void
view_collapse_all_cb(GtkWidget *w, gpointer data)
{
    logCollapseAll();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_edit_cb(GtkWidget *w, gpointer data)
{
    LogRec *lr = logSelected();

    if (lr != 0)    
	startEdit(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_edit_next_cb(GtkWidget *w, gpointer data)
{
    gboolean next = GPOINTER_TO_INT(data);
    LogRec *lr = logSelected();
    
    do
    {
	lr = (next ? logNextError(lr) : logPrevError(lr));
    } while (lr != 0 && !(prefs.edit_warnings || lr->res.code == FR_ERROR));
    
    if (lr != 0)
    {	
	logSetSelected(lr);
	startEdit(lr);
    }
    else
    {
    	message(_("No more errors in log"));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_expand_cb(GtkCTree *tree, GtkCTreeNode *treeNode, gpointer data)
{
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, treeNode);

    lr->expanded = TRUE;    
}

static void
log_collapse_cb(GtkCTree *tree, GtkCTreeNode *treeNode, gpointer data)
{
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, treeNode);

    lr->expanded = FALSE;    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_click_cb(GtkCTree *tree, GtkCTreeNode *treeNode, gint column, gpointer data)
{
#if 0   
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, treeNode);
    
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
	LogRec *lr = logSelected();

	if (lr != 0)
	{
#if 0
	    fprintf(stderr, "log_doubleclick_cb: code=%d file=\"%s\" line=%d\n",
    		lr->res.code, lr->res.file, lr->res.line);
#endif
	    startEdit(lr);
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
uiCreateMenus(GtkWidget *menubar)
{
    GtkWidget *menu;
    
    menu = uiAddMenu(menubar, _("File"));
    uiAddTearoff(menu);
    uiAddButton(menu, _("Open Log..."), file_open_cb, 0, GR_NONE);
    uiAddButton(menu, _("Save Log..."), file_save_cb, 0, GR_NOTEMPTY);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Change directory..."), unimplemented, 0, GR_NOTRUNNING);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Print"), unimplemented, 0, GR_NOTEMPTY);
    uiAddButton(menu, _("Print Settings..."), unimplemented, 0, GR_NONE);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Exit"), file_exit_cb, 0, GR_NONE);
    
    menu = uiAddMenu(menubar, _("Build"));
    buildMenu = menu;
    uiAddTearoff(menu);
    uiAddButton(menu, _("Again"), build_again_cb, 0, GR_AGAIN);
    uiAddButton(menu, _("Stop"), build_stop_cb, 0, GR_RUNNING);
    uiAddSeparator(menu);
#if 0
    uiAddButton(menu, "all", build_cb, "all", GR_NOTRUNNING);
    uiAddButton(menu, "install", build_cb, "install", GR_NOTRUNNING);
    uiAddButton(menu, "clean", build_cb, "clean", GR_NOTRUNNING);
    uiAddSeparator(menu);
    uiAddButton(menu, "random", build_cb, "random", GR_NOTRUNNING);
    uiAddButton(menu, "delay", build_cb, "delay", GR_NOTRUNNING);
    uiAddButton(menu, "targets", build_cb, "targets", GR_NOTRUNNING);
#endif
    
    menu = uiAddMenu(menubar, _("View"));
    uiAddTearoff(menu);
    uiAddButton(menu, _("Clear Log"), view_clear_cb, 0, GR_NOTEMPTY);
    uiAddButton(menu, _("Collapse All"), view_collapse_all_cb, 0, GR_NOTEMPTY);
    uiAddButton(menu, _("Edit"), view_edit_cb, 0, GR_EDITABLE);
    uiAddButton(menu, _("Edit Next Error"), view_edit_next_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    uiAddButton(menu, _("Edit Prev Error"), view_edit_next_cb, GINT_TO_POINTER(FALSE), GR_NOTEMPTY);
    uiAddSeparator(menu);
    uiAddToggle(menu, _("Toolbar"), view_widget_cb, (gpointer)&toolbar, 0, TRUE);
    uiAddToggle(menu, _("Statusbar"), view_widget_cb, (gpointer)&messagebox, 0, TRUE);
    uiAddSeparator(menu);
    uiAddToggle(menu, _("Errors"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_ERRORS),
    	0, logGetFlags() & LF_SHOW_ERRORS);
    uiAddToggle(menu, _("Warnings"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_WARNINGS),
    	0, logGetFlags() & LF_SHOW_WARNINGS);
    uiAddToggle(menu, _("Information"), view_flags_cb, GINT_TO_POINTER(LF_SHOW_INFO),
    	0, logGetFlags() & LF_SHOW_INFO);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Preferences"), view_preferences_cb, 0, GR_NONE);
    
    menu = uiAddMenuRight(menubar, _("Help"));
    uiAddTearoff(menu);
    uiAddButton(menu, _("About Maketool..."), help_about_cb, 0, GR_NONE);
    uiAddButton(menu, _("About make..."), help_about_make_cb, 0, GR_NONE);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Help on..."), unimplemented, 0, GR_NONE);
    uiAddSeparator(menu);
    uiAddButton(menu, _("Tutorial"), unimplemented, 0, GR_NONE);
    uiAddButton(menu, _("Reference Index"), unimplemented, 0, GR_NONE);
    uiAddButton(menu, _("Home Page"), unimplemented, 0, GR_NONE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "all.xpm"
#include "again.xpm"
#include "clean.xpm"
#include "clear.xpm"
#include "next.xpm"
#include "print.xpm"

static void
uiCreateTools()
{
    char tooltip[1024];
    
    uiToolCreate(toolbar, _("Again"), _("Build last target again"),
    	again_xpm, build_again_cb, 0, GR_AGAIN);
    
    uiToolAddSpace(toolbar);

    sprintf(tooltip, _("Build `%s'"), "all");
    uiToolCreate(toolbar, "all", tooltip,
    	all_xpm, build_cb, "all", GR_NOTRUNNING);
    sprintf(tooltip, _("Build `%s'"), "clean");
    uiToolCreate(toolbar, "clean", tooltip,
    	clean_xpm, build_cb, "clean", GR_NOTRUNNING);
    
    uiToolAddSpace(toolbar);
    
    uiToolCreate(toolbar, _("Clear"), _("Clear log"),
    	clear_xpm, view_clear_cb, 0, GR_NOTEMPTY);
    uiToolCreate(toolbar, _("Next"), _("Edit next error or warning"),
    	next_xpm, view_edit_next_cb, GINT_TO_POINTER(TRUE), GR_NOTEMPTY);
    
    uiToolAddSpace(toolbar);
    
    uiToolCreate(toolbar, _("Print"), _("Print log"),
    	print_xpm, unimplemented, 0, GR_NOTEMPTY);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "anim0.xpm"
#include "anim1.xpm"
#include "anim2.xpm"
#include "anim3.xpm"
#include "anim4.xpm"
#include "anim5.xpm"
#include "anim6.xpm"
#include "anim7.xpm"
#include "anim8.xpm"

#define ANIM_INIT(n) \
    animPixmaps[n] = gdk_pixmap_create_from_xpm_d(toplevel->window, \
    	&animMasks[n], 0, PASTE3(anim,n,_xpm));

static void
uiInitAnimPixmaps(void)
{
    ANIM_INIT(0);
    ANIM_INIT(1);
    ANIM_INIT(2);
    ANIM_INIT(3);
    ANIM_INIT(4);
    ANIM_INIT(5);
    ANIM_INIT(6);
    ANIM_INIT(7);
    ANIM_INIT(8);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "maketool.xpm"

static void
uiCreate(void)
{
    GtkWidget *mainvbox, *vbox; /* need 2 for correct spacing */
    GtkWidget *menubar, *logwin;
    GtkTooltips *tooltips;
    GtkWidget *sw;
    GdkPixmap *iconpm;
    GdkBitmap *iconmask = 0;
    static char *titles[1] = { "Log" };
    
    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(toplevel), "Maketool 0.1");
    gtk_widget_set_usize(toplevel, 300, 500);
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

    mainvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(toplevel), mainvbox);
    gtk_container_border_width(GTK_CONTAINER(mainvbox), 0);

    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(mainvbox), menubar, FALSE, FALSE, 0);
    uiCreateMenus(menubar);
    gtk_widget_show(GTK_WIDGET(menubar));
        
    vbox = gtk_vbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(mainvbox), vbox, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);

    toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(GTK_WIDGET(toolbar));
    uiCreateTools(toolbar);
    
    sw = gtk_scrolled_window_new(0, 0);
    gtk_container_border_width(GTK_CONTAINER(sw), 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
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
    logInit(logwin);
    
    messagebox = gtk_hbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), messagebox, FALSE, FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(messagebox), 0);
    gtk_widget_show(GTK_WIDGET(messagebox));
    
    messageent = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(messageent), FALSE);
    gtk_entry_set_text(GTK_ENTRY(messageent), _("Welcome to Maketool"));
    gtk_box_pack_start(GTK_BOX(messagebox), messageent, TRUE, TRUE, 0);   
    gtk_widget_show(messageent);
    
    uiInitAnimPixmaps();

    anim = gtk_pixmap_new(animPixmaps[0], animMasks[0]);
    gtk_box_pack_start(GTK_BOX(messagebox), anim, FALSE, FALSE, 0);   
    gtk_widget_show(anim);

    grey_menu_items();
    listTargets();

    gtk_widget_show(GTK_WIDGET(mainvbox));
    gtk_widget_show(GTK_WIDGET(vbox));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
usage(void)
{
    fprintf(stderr, _("Usage: %s [options] target [target...]\n"), argv0);
    /* TODO: options */
}

static void
parseArgs(int argc, char **argv)
{
    int i;
    estring targs;
    
    argv0 = argv[0];
    estring_init(&targs);
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
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
    /*setlocale(LC_ALL, "");*/
    bindtextdomain("maketool", "."); /* TODO: defines in config.h */
    textdomain("maketool");
    
    gtk_init(&argc, &argv);    
    preferences_init();
    parseArgs(argc, argv);
    uiCreate();
    if (targets != 0)
    	buildStart(targets);
    gtk_main();
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
