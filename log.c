#include <sys/types.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "filter.h"
#include "log.h"

#ifndef GTK_CTREE_IS_EMPTY
#define GTK_CTREE_IS_EMPTY(_ctree_) \
	(gtk_ctree_node_nth(GTK_CTREE(_ctree_), 0) == 0)
#endif

#include "error.xpm"
#include "warning.xpm"
#include "info.xpm"

typedef struct
{
    GdkPixmap *open_pm;
    GdkBitmap *open_mask;
    GdkPixmap *closed_pm;
    GdkBitmap *closed_mask;
} NodeIcons;

static GtkWidget	*logwin;	/* a GtkCTree widget */
static int		numErrors;
static int		numWarnings;
static int		flags = LF_SHOW_INFO|LF_SHOW_WARNINGS|LF_SHOW_ERRORS;
static GList		*log;		/* list of LogRecs */
static LogRec		*currentBuildRec = 0;
static GList		*logPendingLines = 0;
static GList		*logDirectoryStack = 0;
static GdkFont		*fonts[L_MAX];
static GdkColor		foregrounds[L_MAX];
static gboolean		foreground_set[L_MAX];
static GdkColor		backgrounds[L_MAX];
static gboolean		background_set[L_MAX];
static NodeIcons	icons[L_MAX];

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
logAddRec(const char *line, const FilterResult *res)
{
    LogRec *lr;
    
    lr = g_new(LogRec, 1);
    lr->res = *res;
    if (lr->res.file != 0)
	lr->res.file = g_strdup(lr->res.file);	/* TODO: hashtable */
    lr->line = g_strdup(line);
    lr->expanded = TRUE;

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

    log = g_list_append(log, lr);		/* TODO: fix O(N^2) */

    return lr;
}

static void
logDelRec(LogRec *lr)
{
    if (lr->res.file != 0)
    	g_free(lr->res.file);
    g_free(lr->line);
    g_free(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logShowRec(LogRec *lr)
{
    gboolean was_empty = GTK_CTREE_IS_EMPTY(logwin);
    LogSeverity sev = L_INFO;
    GtkCTreeNode *parentNode = (currentBuildRec == 0 ? 0 : currentBuildRec->node);
    
    switch (lr->res.code)
    {
    case FR_UNDEFINED:		/* same as INFORMATION */
    case FR_INFORMATION:
    	/* use default font, fgnd, bgnd */
	if (!(flags & LF_SHOW_INFO))
	    return;
    	break;
    case FR_WARNING:
	sev = L_WARNING;
	if (!(flags & LF_SHOW_WARNINGS))
	    return;
    	break;
    case FR_ERROR:
	sev = L_ERROR;
	if (!(flags & LF_SHOW_ERRORS))
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
    case FR_BUILDSTART:
	currentBuildRec = lr;
	parentNode = 0;
    	break;
    }

    /* TODO: freeze & thaw if it redraws the wrong colour 1st */
    lr->node = gtk_ctree_insert_node(GTK_CTREE(logwin),
    	parentNode,				/* parent */
	(GtkCTreeNode*)0,			/* sibling */
	&lr->line,				/* text[] */
	0,					/* spacing */
	icons[sev].closed_pm,
	icons[sev].closed_mask,			/* pixmap_closed,mask_closed */
	icons[sev].open_pm,
	icons[sev].open_mask,			/* pixmap_opened,mask_opened */
	(parentNode != 0),			/* is_leaf */
	lr->expanded);				/* expanded */
    gtk_ctree_node_set_row_data(GTK_CTREE(logwin), lr->node, (gpointer)lr);
    /* TODO: support different fonts */
    if (foreground_set[sev] != 0)
	gtk_ctree_node_set_foreground(GTK_CTREE(logwin), lr->node, &foregrounds[sev]);
    if (background_set[sev] != 0)
    	gtk_ctree_node_set_background(GTK_CTREE(logwin), lr->node, &backgrounds[sev]);
    
    if (was_empty)
    	grey_menu_items();	/* log window just became non-empty */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logAddLine(const char *line)
{
    FilterResult res;

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
    logShowRec(logAddRec(line, &res));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logClear(void)
{
    /* delete all LogRecs */
    while (log != 0)
    {
    	logDelRec((LogRec *)log->data);
	log = g_list_remove_link(log, log);
    }

    /* clear the log window */
    gtk_clist_clear(GTK_CLIST(logwin));

    /* update sensitivity of menu items */        
    grey_menu_items();
}

gboolean
logIsEmpty(void)
{
    return GTK_CTREE_IS_EMPTY(logwin);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logRepopulate(void)
{
    GList *list;
    
    gtk_clist_freeze(GTK_CLIST(logwin));
    gtk_clist_clear(GTK_CLIST(logwin));
    for (list=log ; list!=0 ; list=list->next)
	logShowRec((LogRec *)list->data);    	
    gtk_clist_thaw(GTK_CLIST(logwin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
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

void
logOpen(const char *file)
{
    FILE *fp;
    char buf[2048];
    
    if ((fp = fopen(file, "r")) == 0)
    {
    	message("%s: %s", file, strerror(errno));
	return;
    }
    
    sprintf(buf, "Log file %s", file);
    logStartBuild(buf);
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	char *x;
	if ((x = strchr(buf, '\n')) != 0)
	    *x = '\0';
	if ((x = strchr(buf, '\r')) != 0)
	    *x = '\0';
    	logAddLine(buf);
    }
    
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

LogRec *
logSelected(void)
{
    if (GTK_CLIST(logwin)->selection == 0)
    	return 0;
	
    return (LogRec *)gtk_ctree_node_get_row_data(GTK_CTREE(logwin), 
    		GTK_CTREE_NODE(GTK_CLIST(logwin)->selection->data));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
logGetFlags(void)
{
    return flags;
}

void
logSetFlags(int f)
{
    flags = f;
    logRepopulate();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
logNumErrors(void)
{
    return numErrors;
}

int
logNumWarnings(void)
{
    return numWarnings;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logInit(GtkWidget *w)
{
    GtkWidget *toplevel = gtk_widget_get_ancestor(w, gtk_window_get_type());
    GdkWindow *win = toplevel->window;
    GdkColormap *colormap = gtk_widget_get_colormap(toplevel);

    /* L_INFO */
    fonts[L_INFO] = 0;
    foreground_set[L_INFO] = FALSE;
    background_set[L_INFO] = FALSE;
    icons[L_INFO].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_INFO].closed_mask, 0, info_xpm);
    icons[L_INFO].open_pm = icons[L_INFO].closed_pm;
    icons[L_INFO].open_mask = icons[L_INFO].closed_mask;
    
    /* L_WARNING */
    fonts[L_WARNING] = 0;
    gdk_color_parse("#ffffc2", &backgrounds[L_WARNING]);
    gdk_colormap_alloc_color(colormap, &backgrounds[L_WARNING], FALSE, TRUE);
    foreground_set[L_WARNING] = FALSE;
    background_set[L_WARNING] = TRUE;
    icons[L_WARNING].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_WARNING].closed_mask, 0, warning_xpm);
    icons[L_WARNING].open_pm = icons[L_WARNING].closed_pm;
    icons[L_WARNING].open_mask = icons[L_WARNING].closed_mask;
    
    /* L_ERROR */
    fonts[L_ERROR] = 0;
    gdk_color_parse("#ffc2c2", &backgrounds[L_ERROR]);
    gdk_colormap_alloc_color(colormap, &backgrounds[L_ERROR], FALSE, TRUE);
    foreground_set[L_ERROR] = FALSE;
    background_set[L_ERROR] = TRUE;
    icons[L_ERROR].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_ERROR].closed_mask, 0, error_xpm);
    icons[L_ERROR].open_pm = icons[L_ERROR].closed_pm;
    icons[L_ERROR].open_mask = icons[L_ERROR].closed_mask;
			
    logwin = w;
    filter_load();
}

void
logSetStyle(LogSeverity s, GdkFont *font, GdkColor *fore, GdkColor *back)
{
    fonts[s] = font;

    if (fore != 0)
    {
    	foreground_set[s] = TRUE;
	foregrounds[s] = *fore;
    }
    else
    {
    	foreground_set[s] = FALSE;
    }
    
    if (back != 0)
    {
    	background_set[s] = TRUE;
	backgrounds[s] = *back;
    }
    else
    {
    	background_set[s] = FALSE;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logStartBuild(const char *message)
{
    FilterResult res;
    
    numErrors = 0;
    numWarnings = 0;
    filter_init();
    
    res.code = FR_BUILDSTART;
    res.file = 0;
    res.line = 0;
    res.column = 0;

    logShowRec(logAddRec(message, &res));
}


void
logEndBuild(const char *target)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
