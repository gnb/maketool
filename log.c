/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2001 Greg Banks
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
#include "ps.h"

CVSID("$Id: log.c,v 1.36 2001-07-25 08:35:22 gnb Exp $");

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
static gboolean     	num_ew_changed = FALSE;
static GList		*log;		/* list of LogRecs */
/*static GList		*log_pending_lines = 0;*/ /*TODO*/
static GList		*log_directory_stack = 0;
static GList		*log_node_stack = 0;	/* stack of LogRec's */
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

/* TODO: hmm, do the directory names need to be strdup()ed ?? */

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

static void
log_change_node(LogRec *lr)
{
    if (log_node_stack == 0)
	log_node_stack = g_list_append(log_node_stack, lr);
    else
    {
	log_node_stack->data = lr;
    }
}

static void
log_push_node(LogRec *lr)
{
    log_node_stack = g_list_prepend(log_node_stack, lr);
}

static void
log_pop_node(void)
{
    if (log_node_stack != 0)
    {
	log_node_stack = g_list_remove_link(log_node_stack, log_node_stack);
    }
}

static void
log_clear_nodes(void)
{
    while (log_node_stack != 0)
    {
	log_node_stack = g_list_remove_link(log_node_stack, log_node_stack);
    }
}


/*
 * Returns the node nearest the bottom of the stack (i.e.
 * the rootmost in the log window's tree representation)
 * with the same directory as the top of the stack. This
 * trick prevents recursive make's which don't change
 * directory from appearing as deeper levels in the tree.
 */
static LogRec *
log_current_unique_node(void)
{
    GList *link;
    LogRec *top;
    
    if (log_node_stack == 0)
    	return 0;
    top = (LogRec *)log_node_stack->data;
    if (log_node_stack->next == 0 ||
    	(top->res.code != FR_PUSHDIR && top->res.code != FR_CHANGEDIR))
	return top;
	
    for (link = log_node_stack->next ; link != 0 ; link = link->next)
    {
    	LogRec *lr = (LogRec *)link->data;
    	if ((lr->res.code != FR_PUSHDIR && lr->res.code != FR_CHANGEDIR) ||
	    strcmp(lr->res.file, top->res.file))
	    return (LogRec*)link->prev->data;
    }
    /*
     * This should never happen 'cos the bottom-most in
     * the stack should always be a FR_BUILDSTART.
     */
    return 0;
}

static LogRec *
log_rootmost_node(void)
{
    GList *tail = g_list_last(log_node_stack);
    return (tail == 0 ? 0 : (LogRec *)tail->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static LogRec *
log_add_rec(const char *line, const FilterResult *res)
{
    LogRec *lr;
    
    lr = g_new(LogRec, 1);
    memset(lr, 0, sizeof(LogRec));
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
	num_ew_changed = TRUE;
    	break;
    case FR_ERROR:
	num_errors++;
	num_ew_changed = TRUE;
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

const char *
log_get_text(const LogRec *lr)
{
    return ((prefs.log_flags & LF_SUMMARISE) && lr->res.summary != 0 ?
    		lr->res.summary : lr->line);
}
		
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
log_show_rec(LogRec *lr)
{
    gboolean was_empty = GTK_CTREE_IS_EMPTY(logwin);
    LogSeverity sev = L_INFO;
    LogRec *parent_rec = log_current_unique_node();
    char *text;
    gboolean is_leaf = TRUE;

    lr->node = 0;
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
	log_clear_nodes();
	log_push_node(lr);
	parent_rec = 0;
	is_leaf = FALSE;
	break;
    case FR_CHANGEDIR:
    	if (prefs.log_flags & LF_INDENT_DIRS)
	{
    	    log_change_node(lr);
	    is_leaf = FALSE;
    	}
	break;
    case FR_PUSHDIR:
    	if (prefs.log_flags & LF_INDENT_DIRS)
	{
    	    log_push_node(lr);
	    is_leaf = FALSE;
	}
	break;
    case FR_POPDIR:
    	if (prefs.log_flags & LF_INDENT_DIRS)
    	    log_pop_node();
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
    
    text = (char *)log_get_text(lr);
    
    /* TODO: freeze & thaw if it redraws the wrong colour 1st */
    lr->node = gtk_ctree_insert_node(GTK_CTREE(logwin),
    	(parent_rec == 0 ? 0 : parent_rec->node),/* parent */
	(GtkCTreeNode*)0,			/* sibling */
	&text,					/* text[] */
	0,					/* spacing */
	icons[sev].closed_pm,
	icons[sev].closed_mask,			/* pixmap_closed,mask_closed */
	icons[sev].open_pm,
	icons[sev].open_mask,			/* pixmap_opened,mask_opened */
	is_leaf,	    			/* is_leaf */
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

static void
log_update_build_start(void)
{
    LogRec *bs;
    estring text;
    int n = 0;
    
    if (!num_ew_changed || (num_errors == 0 && num_warnings == 0))
    	return;
    if ((bs = log_rootmost_node()) == 0)
    	return; /* yeah like that's going to happen */
    assert(bs->node != 0);
    
    estring_init(&text);
    estring_append_string(&text, bs->line);
    estring_append_string(&text, " (");
    if (num_errors > 0)
    {
    	if (n)
	    estring_append_string(&text, ", ");
	estring_append_printf(&text, _("%d errors"), num_errors);
	n++;
    }
    if (num_warnings > 0)
    {
    	if (n)
	    estring_append_string(&text, ", ");
	estring_append_printf(&text, _("%d warnings"), num_warnings);
    	n++;
    }
    estring_append_string(&text, ")");

    gtk_ctree_node_set_text(GTK_CTREE(logwin), bs->node, /*column*/0, text.data);

    estring_free(&text);
    num_ew_changed = FALSE;
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
    	line, (int)res.code, safe_str(res.file), res.line);
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
    	if (res.file != 0 && *res.file != '/' && log_current_dir() != 0)
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
    /* TODO: do this exactly once when loading from file */
    log_update_build_start();
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

/* BEGIN: pinched from gtkclist.c */
/* this defigns the base grid spacing */
#define CELL_SPACING 1
/* returns the row index from a y pixel location in the 
 * context of the clist's voffset */
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
				    ((clist)->row_height + CELL_SPACING))
/* END: pinched from gtkclist.c */

static void
log_repopulate(void)
{
    GList *list;
    LogRec *first = 0;
    
    gtk_clist_freeze(GTK_CLIST(logwin));
    
    /* figure out which log record is the first-visible in the ctree */
    if (!log_is_empty())
    {
	first = (LogRec *)gtk_ctree_node_get_row_data(
    		GTK_CTREE(logwin),
		gtk_ctree_node_nth(
    		    GTK_CTREE(logwin),
		    ROW_FROM_YPIXEL(GTK_CLIST(logwin), 0)));
#if DEBUG > 10
	fprintf(stderr, "old first = \"%s\"\n", first->line);
#endif
    }
    
    /* remove all the rows and build new ones according to new flags */
    gtk_clist_clear(GTK_CLIST(logwin));
    for (list=log ; list!=0 ; list=list->next)
	log_show_rec((LogRec *)list->data);
	
    if (first != 0)
    {
	/* figure out which row is to be the new first-visible */
	GList *firstl = g_list_find(log, first);
	
	for (list = firstl ;
	     list != 0 && ((LogRec *)list->data)->node == 0 ;
	     list = list->next)
	    ;
	if (list == 0)
	    for (list = firstl ;
		 list != 0 && ((LogRec *)list->data)->node == 0 ;
		 list = list->prev)
		;
	if (list != 0)
	{
    	    first = (LogRec *)list->data;
#if DEBUG > 10
	    fprintf(stderr, "new first = \"%s\"\n", first->line);
#endif
    	    gtk_ctree_node_moveto(GTK_CTREE(logwin), first->node, 0,
	    	/* row_align */0.0, /* col_align */0.0);
	    
	}
	    
    }
    
    gtk_clist_thaw(GTK_CLIST(logwin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: rewrite log_save() and log_open() in terms of the
 * existing functions log_apply(), log_start_build(), log_add_line()
 * and new functions log_freeze() and log_thaw().
 */
 
void
log_save(const char *file)
{
    GList *list;
    FILE *fp;
    
    if ((fp = fopen(file, "w")) == 0)
    {
    	/* Note: assumes g_strerror() does i18n of system error messages. */
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
    	/* Note: assumes g_strerror() does i18n of system error messages. */
    	message("%s: %s", file, g_strerror(errno));
	return;
    }
    
    gtk_clist_freeze(GTK_CLIST(logwin));
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
    gtk_clist_thaw(GTK_CLIST(logwin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int 
log_get_indent_level(const LogRec *lr)
{
    int indent = 0;
    GtkCTreeNode *node = lr->node;
    
    for (;;)
    {
    	GtkCTreeNode *parent = GTK_CTREE_ROW(node)->parent;
	
	if (parent == 0)
	    return indent;
	indent++;
	
	node = parent;
    }
    return 0;	/* NOTREACHED */
}


void
log_print(FILE *fp)
{
    GList *list;
    PsDocument *ps;
    int i;
    
    ps = ps_begin(fp);

    /* TODO: other std comments */
    ps_title(ps, "Maketool log");	/* TODO: more details */

    i = 0;
    for (list=log ; list!=0 ; list=list->next)
    {
    	LogRec *lr = (LogRec *)list->data;
	if (lr->node != 0)
	    i++;
    }
    ps_num_lines(ps, i);
    
    for (i=0 ; i<L_MAX ; i++)
    {
    	if (foreground_set[i])
	    ps_foreground(ps, i,
    		(float)foregrounds[i].red / 65535.0,
    		(float)foregrounds[i].green / 65535.0,
    		(float)foregrounds[i].blue / 65535.0);
    	if (background_set[i])
	    ps_background(ps, i,
    		(float)backgrounds[i].red / 65535.0,
    		(float)backgrounds[i].green / 65535.0,
    		(float)backgrounds[i].blue / 65535.0);
    }
    
    ps_prologue(ps);
    
    for (list=log ; list!=0 ; list=list->next)
    {
    	LogRec *lr = (LogRec *)list->data;
	LogSeverity sev = L_INFO;
	
	/*
	 * We only want to print visible log records. LogRec visibility
	 * was determined in log_show_rec() using various rules which
	 * we'd rather not reproduce here.
	 */
	if (lr->node == 0)
	    continue;
    
    	/*
	 * Calculate severity level for display purposes.
	 * TODO: this is already done once by log_show_rec(),
	 * should store the result.
	 */
	switch (lr->res.code)
	{
	case FR_WARNING:
	    sev = L_WARNING;
    	    break;
	case FR_ERROR:
	    sev = L_ERROR;
    	    break;
	}

	ps_line(ps, log_get_text(lr), sev, log_get_indent_level(lr));
    }
    
    ps_end(ps);
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
    if (lr->node != 0)
    {
	gtk_ctree_select(GTK_CTREE(logwin), lr->node);
    	if (gtk_ctree_node_is_visible(GTK_CTREE(logwin), lr->node) != GTK_VISIBILITY_FULL)
	    gtk_ctree_node_moveto(GTK_CTREE(logwin), lr->node, 0, 0.5, 0.0);
    }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_ensure_visible(const LogRec *lr)
{
    if (gtk_ctree_node_is_visible(GTK_CTREE(logwin), lr->node) != GTK_VISIBILITY_FULL)
	gtk_ctree_node_moveto(GTK_CTREE(logwin), lr->node, 0, 0.5, 0.0);
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
    num_ew_changed = FALSE;
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

#define FIRST() \
    	(forward ? log : g_list_last(log))
#define NEXT(l) \
	(forward ? (l)->next : (l)->prev)
    
void
log_apply_after(
    LogApplyFunc func,
    gboolean forward,
    LogRec *start,
    gpointer user_data)
{
    GList *list;
    
    if (start == 0)
    {
    	list = FIRST();
    }
    else
    {
    	list = g_list_find(log, start);
	if (list == 0)
    	    list = FIRST();
	else
	    list = NEXT(list);
    }
    
    for ( ; list!=0 ; list=NEXT(list))
    {
    	LogRec *lr = (LogRec *)list->data;
	
	if (!(*func)(lr, user_data))
	    return;
    }
}

void
log_apply(LogApplyFunc func, gpointer user_data)
{
    log_apply_after(func, TRUE, 0, user_data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
log_get_icon(
    LogSeverity level,
    GdkPixmap **open_pmp,
    GdkBitmap **open_maskp,
    GdkPixmap **closed_pmp,
    GdkBitmap **closed_maskp)
{
    if (open_pmp != 0)
	*open_pmp = icons[level].open_pm;
    if (open_maskp != 0)
	*open_maskp = icons[level].open_mask;
    if (closed_pmp != 0)
	*closed_pmp = icons[level].closed_pm;
    if (closed_maskp != 0)
	*closed_maskp = icons[level].closed_mask;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
