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


static GtkWidget	*logwin;	/* a GtkCTree widget */
static int		numErrors;
static int		numWarnings;
static int		flags = LF_SHOW_INFO|LF_SHOW_WARNINGS|LF_SHOW_ERRORS;
static GList		*log;		/* list of LogRecs */
static GList		*logPendingLines = 0;
static GList		*logDirectoryStack = 0;
static GdkFont		*fonts[L_MAX];
static GdkColor		foregrounds[L_MAX];
static gboolean		foreground_set[L_MAX];
static GdkColor		backgrounds[L_MAX];
static gboolean		background_set[L_MAX];

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
    lr = logAddRec(line, &res);
    log = g_list_append(log, lr);		/* TODO: fix O(N^2) */
    logShowRec(lr);
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
    logwin = w;
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
logStartBuild(const char *target)
{
    numErrors = 0;
    numWarnings = 0;
}

void
logEndBuild(const char *target)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
