#include <sys/types.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "spawn.h"
#include "filter.h"


typedef struct
{
    /* TODO: 2 verbosity levels */
    char *line;
    FilterResult res;
    GtkCTreeNode *node;
} LogRec;

typedef enum
{
    SET_NOTEMPTY,
    SET_NOTRUNNING,
    SET_RUNNING,
    SET_SELECTED,
    SET_AGAIN,

    NUM_SETS
} WidgetSet;

#ifndef GTK_CTREE_IS_EMPTY
#define GTK_CTREE_IS_EMPTY(_ctree_) \
	(gtk_ctree_node_nth(GTK_CTREE(_ctree_), 0) == 0)
#endif

const char	*argv0;
char		**targets;
int		numTargets;
int		currentTarget;
const char	*lastTarget = 0;
GtkWidget	*toplevel;
GtkWidget	*toolbar, *messagebox;
GtkWidget	*logwin, *messageent;
gboolean	showWarnings = TRUE, showErrors = TRUE, showInfo = TRUE;
GList		*widgets[NUM_SETS];
pid_t		currentPid = -1;
gboolean	interrupted = FALSE;
gint		currentInput;
int		currentFd;
int		numErrors;
int		numWarnings;
GList		*log;	/* list of LogRecs */

#define ANIM_MAX 8
GdkPixmap	*animPixmaps[ANIM_MAX+1];
GdkBitmap	*animMasks[ANIM_MAX+1];
GtkWidget	*anim;

GdkFont		*warningFont, *errorFont;
GdkColor	warningForeground, errorForeground;
GdkColor	warningBackground, errorBackground;

#define SPACING 4

#define PASTE3(x,y,z) x##y##z

static LogRec *logSelected(void);
static void handleLine(char *line);


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
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

static void
widgets_set_sensitive(WidgetSet set, gboolean b)
{
    GList *list;
    
    for (list = widgets[set] ; list != 0 ; list = list->next)
    	gtk_widget_set_sensitive(GTK_WIDGET(list->data), b);
}

static void
widgets_add(WidgetSet set, GtkWidget *w)
{
    widgets[set] = g_list_prepend(widgets[set], w);
}

static void
grey_menu_items(void)
{
    gboolean running = (currentPid > 0);
    gboolean empty = GTK_CTREE_IS_EMPTY(logwin);
    gboolean selected = (logSelected() != 0);
    gboolean again = (lastTarget != 0 && !running);

    widgets_set_sensitive(SET_NOTRUNNING, !running);
    widgets_set_sensitive(SET_RUNNING, running);
    widgets_set_sensitive(SET_NOTEMPTY, !empty);
    widgets_set_sensitive(SET_SELECTED, selected);
    widgets_set_sensitive(SET_AGAIN, again);
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

static LogRec *
lr_new(const char *line, const FilterResult *res)
{
    LogRec *lr;
    
    lr = g_new(LogRec, 1);
    lr->res = *res;
    if (lr->res.file != 0)
	lr->res.file = g_strdup(lr->res.file);	/* TODO: hashtable */
    lr->line = g_strdup(line);

    switch (lr->res.code)
    {
    case FR_WARNING:
	numWarnings++;
    	break;
    case FR_ERROR:
	numErrors++;
    	break;
    default:
    	break;
    }

    return lr;
}

static void
lr_delete(LogRec *lr)
{
    if (lr->res.file != 0)
    	g_free(lr->res.file);
    g_free(lr->line);
    g_free(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logAddLine(LogRec *lr)
{
    gboolean was_empty = GTK_CTREE_IS_EMPTY(logwin);
    GdkFont *font = 0;
    GdkColor *fore = 0;
    GdkColor *back = 0;

    switch (lr->res.code)
    {
    case FR_UNDEFINED:		/* same as INFORMATION */
    case FR_INFORMATION:
    	/* use default font, fgnd, bgnd */
	if (!showInfo)
	    return;
    	break;
    case FR_WARNING:
    	font = warningFont;
	fore = &warningForeground;
	back = &warningBackground;
	if (!showWarnings)
	    return;
    	break;
    case FR_ERROR:
    	font = errorFont;
	fore = &errorForeground;
	back = &errorBackground;
	if (!showErrors)
	    return;
    	break;
    case FR_CHANGEDIR:
    	/* TODO: */
    	break;
    case FR_PUSHDIR:
    	/* TODO: */
    	break;
    case FR_POPDIR:
    	/* TODO: */
    	break;
    case FR_PENDING:
    	/* TODO: */
    	break;
    }

    /* TODO: freeze & thaw if it redraws the wrong colour 1st */
    lr->node = gtk_ctree_insert_node(GTK_CTREE(logwin),
    	(GtkCTreeNode*)0,			/* parent */
	(GtkCTreeNode*)0,			/* sibling */
	&lr->line,					/* text[] */
	0,					/* spacing */
	(GdkPixmap *)0, (GdkBitmap *)0,		/* pixmap_closed,mask_closed */
	(GdkPixmap *)0, (GdkBitmap *)0,		/* pixmap_opened,mask_opened */
	TRUE,					/* is_leaf */
	TRUE);					/* expanded */
    gtk_ctree_node_set_row_data(GTK_CTREE(logwin), lr->node, (gpointer)lr);
    if (fore != 0)
	gtk_ctree_node_set_foreground(GTK_CTREE(logwin), lr->node, fore);
    if (back != 0)
    	gtk_ctree_node_set_background(GTK_CTREE(logwin), lr->node, back);
    
    if (was_empty)
    	grey_menu_items();	/* log window just became non-empty */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logClear(void)
{
    /* delete all LogRecs */
    while (log != 0)
    {
    	lr_delete((LogRec *)log->data);
	log = g_list_remove_link(log, log);
    }

    /* clear the log window */
    gtk_clist_clear(GTK_CLIST(logwin));

    /* update sensitivity of menu items */        
    grey_menu_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logRepopulate(void)
{
    GList *list;
    
    gtk_clist_freeze(GTK_CLIST(logwin));
    gtk_clist_clear(GTK_CLIST(logwin));
    for (list=log ; list!=0 ; list=list->next)
	logAddLine((LogRec *)list->data);    	
    gtk_clist_thaw(GTK_CLIST(logwin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logSave(const char *file)
{
    GList *list;
    FILE *fp;
    
    if ((fp = fopen(file, "w")) == 0)
    {
    	message("%s: %s", file, strerror(errno));
	return;
    }
    
    for (list=log ; list!=0 ; list=list->next)
    {
    	LogRec *lr = (LogRec *)list->data;
	
	fputs(lr->line, fp);
	fputc('\n', fp);
    }
    
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logOpen(const char *file)
{
    FILE *fp;
    char buf[2048];
    
    if ((fp = fopen(file, "r")) == 0)
    {
    	message("%s: %s", file, strerror(errno));
	return;
    }
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	char *x;
	if ((x = strchr(buf, '\n')) != 0)
	    *x = '\0';
	if ((x = strchr(buf, '\r')) != 0)
	    *x = '\0';
    	handleLine(buf);
    }
    
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
logSelected(void)
{
    if (GTK_CLIST(logwin)->selection == 0)
    	return 0;
	
    return (LogRec *)gtk_ctree_node_get_row_data(GTK_CTREE(logwin), 
    		GTK_CTREE_NODE(GTK_CLIST(logwin)->selection->data));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reapEdit(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    if (WIFEXITED(status) || WIFSIGNALED(status))
    	fprintf(stderr, "Reaping editor pid %d\n", (int)pid);
}

static void
startEdit(LogRec *lr)
{
    pid_t pid;
    char buf[2048];
    
    if (lr->res.file == 0 || lr->res.file[0] == '\0')
    	return;
	
    sprintf(buf, "nc -noask -line %d %s", lr->res.line, lr->res.file);
    
    if ((pid = fork()) < 0)
    {
    	/* error */
    	perror("fork");
    }
    else if (pid == 0)
    {
    	/* child */
	execlp("/bin/sh", "/bin/sh", "-c", buf, 0);
	perror("execlp");
    }
    else
    {
    	/* parent */
	message("Editing %s (line %d)", lr->res.file, lr->res.line);
	spawn_add_reap_func(pid, reapEdit, (gpointer)0);
    }
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
	
	if (numErrors > 0)
	    sprintf(errStr, ", %d errors", numErrors);
	else
	    errStr[0] = '\0';
	    
	if (numWarnings > 0)
	    sprintf(warnStr, ", %d warnings", numWarnings);
	else
	    warnStr[0] = '\0';

	if (interrupted)
	    strcpy(intStr, " (interrupted)");
	else
	    intStr[0] = '\0';
	    
	message("Finished making %s%s%s%s", target, errStr, warnStr, intStr);
	
	if (currentPid == pid)
	    currentPid = -1;
	gtk_input_remove(currentInput);
	close(currentFd);
	anim_stop();
	grey_menu_items();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GList *pendingLines = 0;
static GList *directoryStack = 0;

static void
handleLine(char *line)
{
    FilterResult res;
    LogRec *lr;

    res.file = 0;
    res.line = 0;
    res.column = 0;
    filter_apply(line, &res);
#if DEBUG
    fprintf(stderr, "filter_apply: \"%s\" -> %d \"%s\" %d\n",
    	line, (int)res.code, res.file, res.line);
#endif
    if (res.code == FR_UNDEFINED)
    	res.code = FR_INFORMATION;
    lr = lr_new(line, &res);
    log = g_list_append(log, lr);		/* TODO: fix O(N^2) */
    logAddLine(lr);
}


static void
handleData(char *buf, int len)
{
    static char linebuf[1024];
    static int nleftover = 0;
    
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
	handleLine(linebuf);
	nleftover = 0;
	len -= (p - buf);
	buf = ++p;
    }
}


static void
handleInput(gpointer data, gint source, GdkInputCondition condition)
{
    if (source == currentFd && condition == GDK_INPUT_READ)
    {
    	int nremain = 0;
	
	if (ioctl(currentFd, FIONREAD, &nremain) < 0)
	{
	    perror("ioctl(FIONREAD)");
	    return;
	}
	
	while (nremain > 0)
	{
	    char buf[1025];
	    int n = read(currentFd, buf, MIN(sizeof(buf)-1, nremain));
	    nremain -= n;
	    buf[n] = '\0';		/* so we can use str*() calls */
	    handleData(buf, n);
	}
    }
}


#define READ 0
#define WRITE 1
#define STDOUT 1
#define STDERR 2
void
buildStart(const char *target)
{
    pid_t pid;
    int pipefds[2];
    char buf[2048];
    
    sprintf(buf, "make %s", target);
    
    
    if (pipe(pipefds) < 0)
    {
    	perror("pipe");
    	return;
    }
    
    if ((pid = fork()) < 0)
    {
    	/* error */
    	perror("fork");
    }
    else if (pid == 0)
    {
    	/* child */
	close(pipefds[READ]);
	dup2(pipefds[WRITE], STDOUT);
	dup2(pipefds[WRITE], STDERR);
	close(pipefds[WRITE]);
	
	execlp("/bin/sh", "/bin/sh", "-c", buf, 0);
	perror("execlp");
    }
    else
    {
    	/* parent */
	message("Making %s", target);
	lastTarget = target;
	spawn_add_reap_func(pid, reapMake, (gpointer)target);
	currentPid = pid;
	interrupted = FALSE;
	currentFd = pipefds[READ];
	close(pipefds[WRITE]);
	filter_init();
	numErrors = 0;
	numWarnings = 0;
	currentInput = gdk_input_add(currentFd,
			    GDK_INPUT_READ, handleInput, (gpointer)0);
	anim_start();
	grey_menu_items();
    }
}
#undef READ
#undef WRITE
#undef STDOUT
#undef STDERR

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_exit_cb(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
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

static GtkWidget *
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

static void
file_open_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = uiCreateFileSel("Open Log File", logOpen, "make.log");

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_save_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    	filesel = uiCreateFileSel("Save Log File", logSave, "make.log");

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
    gboolean *flagp = (gboolean *)data;
    
    *flagp = GTK_CHECK_MENU_ITEM(w)->active;
    logRepopulate();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
view_clear_cb(GtkWidget *w, gpointer data)
{
    logClear();
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
log_click_cb(GtkCTree *tree, GtkCTreeNode *treeNode, gint column, gpointer data)
{
    LogRec *lr = (LogRec *)gtk_ctree_node_get_row_data(tree, treeNode);
    
    fprintf(stderr, "log_click_cb: code=%d file=\"%s\" line=%d\n",
    	lr->res.code, lr->res.file, lr->res.line);
    grey_menu_items();
}

static void
log_doubleclick_cb(GtkWidget *w, GdkEvent *event, gpointer data)
{
    LogRec *lr;
    
    if (event->type == GDK_2BUTTON_PRESS) 
    {
	lr = logSelected();

	if (lr != 0)
	{
	    fprintf(stderr, "log_doubleclick_cb: code=%d file=\"%s\" line=%d\n",
    		lr->res.code, lr->res.file, lr->res.line);
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

static GtkWidget *
uiAddMenu(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, FALSE);
}

static GtkWidget *
uiAddMenuRight(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, TRUE);
}

static GtkWidget *
uiAddButton(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(label);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    gtk_widget_show(item);
    return item;
}

static GtkWidget *
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


static GtkWidget *
uiAddSeparator(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
uiCreateMenus(GtkWidget *menubar)
{
    GtkWidget *menu;
    GtkWidget *item;
    
    menu = uiAddMenu(menubar, "File");
    uiAddTearoff(menu);
    uiAddButton(menu, "Open Log...", file_open_cb, 0);
    item = uiAddButton(menu, "Save Log...", file_save_cb, 0);
    widgets_add(SET_NOTEMPTY, item);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "Change Makefile...", unimplemented, 0);
    widgets_add(SET_NOTRUNNING, item);
    item = uiAddButton(menu, "Change directory...", unimplemented, 0);
    widgets_add(SET_NOTRUNNING, item);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "Print", unimplemented, 0);
    widgets_add(SET_NOTEMPTY, item);
    uiAddButton(menu, "Print Settings...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Exit", file_exit_cb, 0);
    
    menu = uiAddMenu(menubar, "Build");
    uiAddTearoff(menu);
    item = uiAddButton(menu, "Again", build_again_cb, 0);
    widgets_add(SET_AGAIN, item);
    item = uiAddButton(menu, "Stop", build_stop_cb, 0);
    widgets_add(SET_RUNNING, item);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "all", build_cb, "all");
    widgets_add(SET_NOTRUNNING, item);
    item = uiAddButton(menu, "install", build_cb, "install");
    widgets_add(SET_NOTRUNNING, item);
    item = uiAddButton(menu, "clean", build_cb, "clean");
    widgets_add(SET_NOTRUNNING, item);
    uiAddSeparator(menu);
#if 1
    item = uiAddButton(menu, "random", build_cb, "random");
    widgets_add(SET_NOTRUNNING, item);
    item = uiAddButton(menu, "delay", build_cb, "delay");
    widgets_add(SET_NOTRUNNING, item);
    item = uiAddButton(menu, "targets", build_cb, "targets");
    widgets_add(SET_NOTRUNNING, item);
#endif
    
    menu = uiAddMenu(menubar, "View");
    uiAddTearoff(menu);
    item = uiAddButton(menu, "Clear Log", view_clear_cb, 0);
    widgets_add(SET_NOTEMPTY, item);
    item = uiAddButton(menu, "Edit", view_edit_cb, 0);
    widgets_add(SET_SELECTED, item);
    item = uiAddButton(menu, "Edit Next Error", unimplemented, 0);
    widgets_add(SET_NOTEMPTY, item);
    item = uiAddButton(menu, "Edit Prev Error", unimplemented, 0);
    widgets_add(SET_NOTEMPTY, item);
    uiAddSeparator(menu);
    uiAddToggle(menu, "Toolbar", view_widget_cb, (gpointer)&toolbar, 0, TRUE);
    uiAddToggle(menu, "Messages", view_widget_cb, (gpointer)&messagebox, 0, TRUE);
    uiAddSeparator(menu);
    uiAddToggle(menu, "Errors", view_flags_cb, (gpointer)&showErrors, 0, showErrors);
    uiAddToggle(menu, "Warnings", view_flags_cb, (gpointer)&showWarnings, 0, showWarnings);
    uiAddToggle(menu, "Information", view_flags_cb, (gpointer)&showInfo, 0, showInfo);
    uiAddSeparator(menu);
    uiAddButton(menu, "Preferences", unimplemented, 0);
    
    menu = uiAddMenuRight(menubar, "Help");
    uiAddTearoff(menu);
    uiAddButton(menu, "About Maketool...", unimplemented, 0);
    uiAddButton(menu, "About make...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Help on...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Tutorial", unimplemented, 0);
    uiAddButton(menu, "Reference Index", unimplemented, 0);
    uiAddButton(menu, "Home Page", unimplemented, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GtkWidget *
uiToolCreate(
    GtkWidget *toolbar,
    const char *name,
    const char *tooltip,
    char **pixmap_xpm,
    GtkSignalFunc callback,
    gpointer user_data)
{
    GdkPixmap *pm = 0;
    GdkBitmap *mask = 0;

    pm = gdk_pixmap_create_from_xpm_d(toplevel->window, &mask, 0, pixmap_xpm);
    return gtk_toolbar_append_item(
    	GTK_TOOLBAR(toolbar),
    	name,
	tooltip,
	0,				/* tooltip_private_text */
    	gtk_pixmap_new(pm, mask),
	callback,
	user_data);
}

static void
uiToolAddSpace(GtkWidget *toolbar)
{
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "new.xpm"

static void
uiCreateTools()
{
    GtkWidget *item;
    
    item = uiToolCreate(toolbar, "Again", "Build last target again",
    	new_xpm, build_again_cb, 0);
    widgets_add(SET_AGAIN, item);
    
    uiToolAddSpace(toolbar);

    item = uiToolCreate(toolbar, "all", "Build `all'",
    	new_xpm, build_cb, "all");
    widgets_add(SET_NOTRUNNING, item);
    item = uiToolCreate(toolbar, "clean", "Build `clean'",
    	new_xpm, build_cb, "clean");
    widgets_add(SET_NOTRUNNING, item);
    
    uiToolAddSpace(toolbar);
    
    item = uiToolCreate(toolbar, "Clear", "Clear log of messages",
    	new_xpm, view_clear_cb, 0);
    widgets_add(SET_NOTEMPTY, item);
    item = uiToolCreate(toolbar, "Next", "Edit next error or warning",
    	new_xpm, unimplemented, 0);
    widgets_add(SET_NOTEMPTY, item);
    
    uiToolAddSpace(toolbar);
    
    item = uiToolCreate(toolbar, "Print", "Print messages",
    	new_xpm, unimplemented, 0);
    widgets_add(SET_NOTEMPTY, item);
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

static void
uiInitFontsAndColors(void)
{
    GdkColormap *colormap = gtk_widget_get_colormap(toplevel);
    
    warningFont = 0;
    errorFont = 0;
    
    gdk_color_parse("yellow", &warningBackground);
    gdk_colormap_alloc_color(colormap, &warningBackground, FALSE, TRUE);

    gdk_color_parse("red", &errorBackground);
    gdk_colormap_alloc_color(colormap, &errorBackground, FALSE, TRUE);

    gdk_color_parse("black", &errorForeground);
    gdk_colormap_alloc_color(colormap, &errorForeground, FALSE, TRUE);

    gdk_color_parse("black", &warningForeground);
    gdk_colormap_alloc_color(colormap, &warningForeground, FALSE, TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
uiCreate(void)
{
    GtkWidget *mainvbox, *vbox; /* need 2 for correct spacing */
    GtkWidget *menubar;
    GtkTooltips *tooltips;
    GtkWidget *sw;
    static char *titles[1] = { "Log" };
    
    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(toplevel), "Maketool 0.1");
    gtk_widget_set_usize(toplevel, 300, 500);
    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
    	GTK_SIGNAL_FUNC(file_exit_cb), NULL);
    gtk_container_border_width(GTK_CONTAINER(toplevel), 0);
    gtk_widget_show(GTK_WIDGET(toplevel));
    
    uiInitFontsAndColors();
    
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
    uiCreateTools(toolbar);
    gtk_widget_show(GTK_WIDGET(toolbar));
    
    sw = gtk_scrolled_window_new(0, 0);
    gtk_container_border_width(GTK_CONTAINER(sw), 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    gtk_widget_show(GTK_WIDGET(sw));

    logwin = gtk_ctree_new_with_titles(1, 0, titles);
    gtk_ctree_set_line_style(GTK_CTREE(logwin), GTK_CTREE_LINES_NONE);
    gtk_clist_column_titles_hide(GTK_CLIST(logwin));
    gtk_clist_set_column_width(GTK_CLIST(logwin), 0, 400);
    gtk_clist_set_column_auto_resize(GTK_CLIST(logwin), 0, TRUE);
    gtk_signal_connect(GTK_OBJECT(logwin), "tree-select-row", 
    	GTK_SIGNAL_FUNC(log_click_cb), 0);
    gtk_signal_connect(GTK_OBJECT(logwin), "button_press_event", 
    	GTK_SIGNAL_FUNC(log_doubleclick_cb), 0);
    gtk_container_add(GTK_CONTAINER(sw), logwin);
    gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(sw),
    	GTK_CLIST(logwin)->hadjustment);
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(sw),
    	GTK_CLIST(logwin)->vadjustment);
    gtk_widget_show(logwin);
    
    messagebox = gtk_hbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), messagebox, FALSE, FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(messagebox), 0);
    gtk_widget_show(GTK_WIDGET(messagebox));
    
    messageent = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(messageent), FALSE);
    gtk_entry_set_text(GTK_ENTRY(messageent), "Welcome to Maketool");
    gtk_box_pack_start(GTK_BOX(messagebox), messageent, TRUE, TRUE, 0);   
    gtk_widget_show(messageent);
    
    uiInitAnimPixmaps();

    anim = gtk_pixmap_new(animPixmaps[0], animMasks[0]);
    gtk_box_pack_start(GTK_BOX(messagebox), anim, FALSE, FALSE, 0);   
    gtk_widget_show(anim);

    grey_menu_items();

    gtk_widget_show(GTK_WIDGET(mainvbox));
    gtk_widget_show(GTK_WIDGET(vbox));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [options] target [target...]\n", argv0);
    /* TODO: options */
}

static void
parseArgs(int argc, char **argv)
{
    int i;
    
    argv0 = argv[0];
    
    numTargets = 0;
    currentTarget = 0;
    targets = (char**)malloc(sizeof(char*) * argc);
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    /* TODO: options */
	    usage();
	}
	else
	{
	    targets[numTargets++] = argv[i];
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    parseArgs(argc, argv);
    uiCreate();
    filter_load();
    gtk_main();
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
