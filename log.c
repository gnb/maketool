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
#include "filter.h"
#include "log.h"
#include "util.h"

CVSID("$Id: log.c,v 1.12 1999-05-30 11:24:39 gnb Exp $");

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
/*static GList		*logPendingLines = 0;*/ /*TODO*/
static GList		*logDirectoryStack = 0;
static GdkFont		*fonts[L_MAX];
static GdkColor		foregrounds[L_MAX];
static gboolean		foreground_set[L_MAX];
static GdkColor		backgrounds[L_MAX];
static gboolean		background_set[L_MAX];
static NodeIcons	icons[L_MAX];

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
logChangeDir(const char *dir)
{
    if (logDirectoryStack == 0)
	logDirectoryStack = g_list_append(logDirectoryStack, g_strdup(dir));
    else
    {
    	g_free((char*)logDirectoryStack->data);
	logDirectoryStack->data = g_strdup(dir);
    }
}

static void
logPushDir(const char *dir)
{
    logDirectoryStack = g_list_prepend(logDirectoryStack, g_strdup(dir));
}

static void
logPopDir(void)
{
    if (logDirectoryStack != 0)
    {
    	g_free((char*)logDirectoryStack->data);
	logDirectoryStack = g_list_remove_link(logDirectoryStack, logDirectoryStack);
    }
}

static void
logClearDirs(void)
{
    while (logDirectoryStack != 0)
    {
    	g_free((char*)logDirectoryStack->data);
	logDirectoryStack = g_list_remove_link(logDirectoryStack, logDirectoryStack);
    }
}

static const char *
logCurrentDir(void)
{
    return (logDirectoryStack == 0 ? 0 : (const char *)logDirectoryStack->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
logAddRec(const char *line, const FilterResult *res)
{
    LogRec *lr;
    
    lr = g_new(LogRec, 1);
    lr->res = *res;
    if (lr->res.file == 0 || *lr->res.file == '\0')
    	lr->res.file = 0;
    else
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
    case FR_BUILDSTART:
	currentBuildRec = lr;
	parentNode = 0;
    	break;
    default:
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

LogRec *
logAddLine(const char *line)
{
    FilterResult res;
    LogRec *lr;
    estring fullpath;

    estring_init(&fullpath);
    res.file = 0;
    res.line = 0;
    res.column = 0;
    filter_apply(line, &res);
#if DEBUG
    fprintf(stderr, "filter_apply: \"%s\" -> %d \"%s\" %d\n",
    	line, (int)res.code, res.file, res.line);
#endif

    if (res.file != 0 && *res.file == '\0')
    	res.file = 0;
	
    switch (res.code)
    {
    case FR_UNDEFINED:
    	res.code = FR_INFORMATION;
    	break;
    case FR_CHANGEDIR:
    	logChangeDir(res.file);
    	break;
    case FR_PUSHDIR:
	logPushDir(res.file);
    	break;
    case FR_POPDIR:
	logPopDir();
    	break;
    case FR_PENDING:
    	/* TODO: */
    	break;
    default:
    	if (res.file != 0 && logCurrentDir() != 0)
	{
	    estring_append_string(&fullpath, logCurrentDir());
	    estring_append_char(&fullpath, '/');
	    estring_append_string(&fullpath, res.file);
	    res.file = fullpath.data;
	}
    	break;
    }
    
    lr = logAddRec(line, &res);
    logShowRec(lr);
    estring_free(&fullpath);
    return lr;
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

void
logCollapseAll(void)
{
    GList *list;
    
    for (list = log ; list != 0 ; list = list->next)
    {
    	LogRec *lr = (LogRec *)list->data;
	
	if (lr->res.code == FR_BUILDSTART)
	    gtk_ctree_collapse(GTK_CTREE(logwin), lr->node);
    }
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
    	message("%s: %s", file, g_strerror(errno));
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
    	message("%s: %s", file, g_strerror(errno));
	return;
    }
    
    sprintf(buf, _("Log file %s"), file);
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

void
logSetSelected(LogRec *lr)
{
    gtk_ctree_select(GTK_CTREE(logwin), lr->node);
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logStartBuild(const char *message)
{
    FilterResult res;
    
    logClearDirs();
    
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

static LogRec *
logFindErrorAux(LogRec *lr, gboolean forward)
{
    GList *list;
    
    if (lr == 0)
    	/* begin at start or end */
    	list = (forward ? log : g_list_last(log));
    else
    {
    	/* skip the current logrec */
    	list = g_list_find(log, lr);
	list = (forward ? list->next : list->prev);
    }    

    for ( ; list != 0 ; list = (forward ? list->next : list->prev))
    {
    	lr = (LogRec *)list->data;
	if ((lr->res.code == FR_ERROR || lr->res.code == FR_WARNING) &&
	     lr->res.file != 0)
	    return lr;
    }
    return 0;
}

LogRec *
logNextError(LogRec *lr)
{
    return logFindErrorAux(lr, TRUE);
}

LogRec *
logPrevError(LogRec *lr)
{
    return logFindErrorAux(lr, FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
