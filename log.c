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

CVSID("$Id: log.c,v 1.17 1999-09-05 11:39:31 gnb Exp $");

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
static int		num_errors;
static int		num_warnings;
static GList		*log;		/* list of LogRecs */
static LogRec		*current_build_rec = 0;
/*static GList		*log_pending_lines = 0;*/ /*TODO*/
static GList		*log_directory_stack = 0;
static GdkFont		*fonts[L_MAX];
static GdkColor		foregrounds[L_MAX];
static gboolean		foreground_set[L_MAX];
static GdkColor		backgrounds[L_MAX];
static gboolean		background_set[L_MAX];
static NodeIcons	icons[L_MAX];

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
filter_result_init(FilterResult *res)
{
    res->code = FR_UNDEFINED;
    res->file = 0;
    res->line = 0;
    res->column = 0;
    res->summary = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_change_dir(const char *dir)
{
    if (log_directory_stack == 0)
	log_directory_stack = g_list_append(log_directory_stack, g_strdup(dir));
    else
    {
    	g_free((char*)log_directory_stack->data);
	log_directory_stack->data = g_strdup(dir);
    }
}

static void
log_push_dir(const char *dir)
{
    log_directory_stack = g_list_prepend(log_directory_stack, g_strdup(dir));
}

static void
log_pop_dir(void)
{
    if (log_directory_stack != 0)
    {
    	g_free((char*)log_directory_stack->data);
	log_directory_stack = g_list_remove_link(log_directory_stack, log_directory_stack);
    }
}

static void
log_clear_dirs(void)
{
    while (log_directory_stack != 0)
    {
    	g_free((char*)log_directory_stack->data);
	log_directory_stack = g_list_remove_link(log_directory_stack, log_directory_stack);
    }
}

static const char *
log_current_dir(void)
{
    return (log_directory_stack == 0 ? 0 : (const char *)log_directory_stack->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
log_add_rec(const char *line, const FilterResult *res)
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
	num_warnings++;
    	break;
    case FR_ERROR:
	num_errors++;
    	break;
    default:
    	break;
    }

    log = g_list_append(log, lr);		/* TODO: fix O(N^2) */

    return lr;
}

static void
log_del_rec(LogRec *lr)
{
    if (lr->res.file != 0)
    	g_free(lr->res.file);
    g_free(lr->line);
    g_free(lr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_show_rec(LogRec *lr)
{
    gboolean was_empty = GTK_CTREE_IS_EMPTY(logwin);
    LogSeverity sev = L_INFO;
    GtkCTreeNode *parent_node = (current_build_rec == 0 ? 0 : current_build_rec->node);
    char *text;
    
    switch (lr->res.code)
    {
    case FR_UNDEFINED:		/* same as INFORMATION */
    case FR_INFORMATION:
    	/* use default font, fgnd, bgnd */
	if (!(prefs.log_flags & LF_SHOW_INFO))
	    return;
    	break;
    case FR_WARNING:
	sev = L_WARNING;
	if (!(prefs.log_flags & LF_SHOW_WARNINGS))
	    return;
    	break;
    case FR_ERROR:
	sev = L_ERROR;
	if (!(prefs.log_flags & LF_SHOW_ERRORS))
	    return;
    	break;
    case FR_BUILDSTART:
	current_build_rec = lr;
	parent_node = 0;
    	break;
    default:
    	break;
    }

    text = lr->line;
    if ((prefs.log_flags & LF_SUMMARISE) && lr->res.summary != 0)
    {
	if (*lr->res.summary == '\0')
	    return; 	/* summary="" allows summarised lines to disappear */
	else
	    text = lr->res.summary;
    }
    
    text = ((prefs.log_flags & LF_SUMMARISE) && lr->res.summary != 0 ?
    		lr->res.summary : lr->line);
    
    /* TODO: freeze & thaw if it redraws the wrong colour 1st */
    lr->node = gtk_ctree_insert_node(GTK_CTREE(logwin),
    	parent_node,				/* parent */
	(GtkCTreeNode*)0,			/* sibling */
	&text,					/* text[] */
	0,					/* spacing */
	icons[sev].closed_pm,
	icons[sev].closed_mask,			/* pixmap_closed,mask_closed */
	icons[sev].open_pm,
	icons[sev].open_mask,			/* pixmap_opened,mask_opened */
	(parent_node != 0),			/* is_leaf */
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
log_add_line(const char *line)
{
    FilterResult res;
    LogRec *lr;
    estring fullpath;

    estring_init(&fullpath);
    filter_result_init(&res);
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
    	log_change_dir(res.file);
    	break;
    case FR_PUSHDIR:
	log_push_dir(res.file);
    	break;
    case FR_POPDIR:
	log_pop_dir();
    	break;
    case FR_PENDING:
    	/* TODO: */
    	break;
    default:
    	if (res.file != 0 && log_current_dir() != 0)
	{
	    estring_append_string(&fullpath, log_current_dir());
	    estring_append_char(&fullpath, '/');
	    estring_append_string(&fullpath, res.file);
	    res.file = fullpath.data;
	}
    	break;
    }
    
    lr = log_add_rec(line, &res);
    log_show_rec(lr);
    estring_free(&fullpath);
    return lr;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_clear(void)
{
    /* delete all LogRecs */
    while (log != 0)
    {
    	log_del_rec((LogRec *)log->data);
	log = g_list_remove_link(log, log);
    }

    /* clear the log window */
    gtk_clist_clear(GTK_CLIST(logwin));

    /* update sensitivity of menu items */        
    grey_menu_items();
}

gboolean
log_is_empty(void)
{
    return GTK_CTREE_IS_EMPTY(logwin);
}

void
log_collapse_all(void)
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
log_repopulate(void)
{
    GList *list;
    
    gtk_clist_freeze(GTK_CLIST(logwin));
    gtk_clist_clear(GTK_CLIST(logwin));
    for (list=log ; list!=0 ; list=list->next)
	log_show_rec((LogRec *)list->data);    	
    gtk_clist_thaw(GTK_CLIST(logwin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_save(const char *file)
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
log_open(const char *file)
{
    FILE *fp;
    int len;
    estring leftover;
    char *startstr, buf[1025];

    
    if ((fp = fopen(file, "r")) == 0)
    {
    	message("%s: %s", file, g_strerror(errno));
	return;
    }
    
    startstr = g_strdup_printf(_("Log file %s"), file);
    log_start_build(startstr);
    g_free(startstr);


    /*
     * TODO: reuse commonality between this code
     * and handle_input() in main.c.
     */
    estring_init(&leftover);
    while ((len = fread(buf, 1, sizeof(buf)-1, fp)) > 0)
    {
    	char *start = buf, *end;

    	buf[len] = '\0';
	while (len > 0)
	{
	    end = strchr(start, '\n');
	    if (end == 0)
	    {
    		/* only a part of a line left - append to leftover */
		estring_append_string(&leftover, start);
		len = 0;
	    }
	    else
	    {
    		/* got an end-of-line - isolate the line & feed it to handle_line() */
		*end = '\0';
		estring_append_string(&leftover, start);

    		log_add_line(leftover.data);

		estring_truncate(&leftover);
		len -= (end - start);
		start = ++end;
    	    }
	}
    }
    /* handle case where last line is not terminated with '\n' */
    if (leftover.length > 0)
    	log_add_line(leftover.data);
    
    estring_free(&leftover);
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

LogRec *
log_selected(void)
{
    if (GTK_CLIST(logwin)->selection == 0)
    	return 0;
	
    return (LogRec *)gtk_ctree_node_get_row_data(GTK_CTREE(logwin), 
    		GTK_CTREE_NODE(GTK_CLIST(logwin)->selection->data));
}

void
log_set_selected(LogRec *lr)
{
    gtk_ctree_select(GTK_CTREE(logwin), lr->node);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
log_get_flags(void)
{
    return prefs.log_flags;
}

void
log_set_flags(int f)
{
    prefs.log_flags = f;
    log_repopulate();
    preferences_save();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
log_num_errors(void)
{
    return num_errors;
}

int
log_num_warnings(void)
{
    return num_warnings;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
_log_load_color(
    GdkColormap *colormap,
    const char *color_name,
    GdkColor *col)
{
    if (color_name == 0)
    	return FALSE;
	
    if (!gdk_color_parse(color_name, col))
    	return FALSE;
	
    if (!gdk_colormap_alloc_color(colormap, col, FALSE, TRUE))
    	return FALSE;
	
    return TRUE;
}
    
static void
_log_setup_colors(void)
{
    GdkColormap *colormap = gtk_widget_get_colormap(toplevel);
    
    /* L_INFO */
    foreground_set[L_INFO] = _log_load_color(colormap, prefs.colors[COL_FG_INFO],
    	    	    	    	&foregrounds[L_INFO]);
    background_set[L_INFO] = _log_load_color(colormap, prefs.colors[COL_BG_INFO],
    	    	    	    	&backgrounds[L_INFO]);
    
    /* L_WARNING */
    foreground_set[L_WARNING] = _log_load_color(colormap, prefs.colors[COL_FG_WARNING],
    	    	    	    	&foregrounds[L_WARNING]);
    background_set[L_WARNING] = _log_load_color(colormap, prefs.colors[COL_BG_WARNING],
    	    	    	    	&backgrounds[L_WARNING]);
    
    /* L_ERROR */
    foreground_set[L_ERROR] = _log_load_color(colormap, prefs.colors[COL_FG_ERROR],
    	    	    	    	&foregrounds[L_ERROR]);
    background_set[L_ERROR] = _log_load_color(colormap, prefs.colors[COL_BG_ERROR],
    	    	    	    	&backgrounds[L_ERROR]);
}

static void
_log_free_colors(void)
{
    GdkColormap *colormap = gtk_widget_get_colormap(toplevel);
    
    if (foreground_set[L_INFO])
	gdk_colormap_free_colors(colormap, &foregrounds[L_INFO], 1);
    if (background_set[L_INFO])
	gdk_colormap_free_colors(colormap, &backgrounds[L_INFO], 1);

    if (foreground_set[L_WARNING])
	gdk_colormap_free_colors(colormap, &foregrounds[L_INFO], 1);
    if (background_set[L_WARNING])
	gdk_colormap_free_colors(colormap, &backgrounds[L_INFO], 1);

    if (foreground_set[L_ERROR])
	gdk_colormap_free_colors(colormap, &foregrounds[L_INFO], 1);
    if (background_set[L_ERROR])
	gdk_colormap_free_colors(colormap, &backgrounds[L_INFO], 1);
}

void
log_init(GtkWidget *w)
{
    GdkWindow *win = toplevel->window;
    
    /* L_INFO */
    fonts[L_INFO] = 0;
    icons[L_INFO].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_INFO].closed_mask, 0, info_xpm);
    icons[L_INFO].open_pm = icons[L_INFO].closed_pm;
    icons[L_INFO].open_mask = icons[L_INFO].closed_mask;
    
    /* L_WARNING */
    fonts[L_WARNING] = 0;
    icons[L_WARNING].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_WARNING].closed_mask, 0, warning_xpm);
    icons[L_WARNING].open_pm = icons[L_WARNING].closed_pm;
    icons[L_WARNING].open_mask = icons[L_WARNING].closed_mask;
    
    /* L_ERROR */
    fonts[L_ERROR] = 0;
    icons[L_ERROR].closed_pm = gdk_pixmap_create_from_xpm_d(win,
    			&icons[L_ERROR].closed_mask, 0, error_xpm);
    icons[L_ERROR].open_pm = icons[L_ERROR].closed_pm;
    icons[L_ERROR].open_mask = icons[L_ERROR].closed_mask;

    _log_setup_colors();
			
    logwin = w;
    filter_load();
}

void
log_colors_changed(void)
{
    _log_free_colors();
    _log_setup_colors();
    log_repopulate();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_start_build(const char *message)
{
    FilterResult res;
    
    log_clear_dirs();
    
    num_errors = 0;
    num_warnings = 0;
    filter_init();
    
    filter_result_init(&res);
    res.code = FR_BUILDSTART;

    log_show_rec(log_add_rec(message, &res));
}


void
log_end_build(const char *target)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
log_find_error_aux(LogRec *lr, gboolean forward)
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
log_next_error(LogRec *lr)
{
    return log_find_error_aux(lr, TRUE);
}

LogRec *
log_prev_error(LogRec *lr)
{
    return log_find_error_aux(lr, FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
