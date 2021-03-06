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

#ifndef _LOG_H_
#define _LOG_H_

#include "common.h"
#include "ui.h"
#include "filter.h"
#include "progress.h"
#include "maketool_task.h"

typedef struct
{
    int refcount;
    unsigned int num_dirs;
    const char **dirs;
} LogContext;

typedef struct
{
    char *line;
    FilterResult res;
    GtkCTreeNode *node;
    gboolean expanded;
    LogContext *context;    /* may be null */
} LogRec;

typedef enum
{
    L_INFO, L_WARNING, L_ERROR, L_SUMMARY,
    
    L_MAX
} LogSeverity;

void log_init(GtkWidget *);
void log_set_count_callback(void (*log_count_callback)(int, int));

void log_colors_changed(void);

void log_clear(void);
void log_collapse_all(void);
gboolean log_is_empty(void);
void log_start_build(const char *message);
void log_end_build(const char *target);

/*
 * Generates full pathnames for source files which currently
 * exist.  Returns a new null-terminated array of new strings,
 * suitable for freeing with g_strfreev().
 */
char **log_get_filenames(LogRec *lr);

LogRec *log_selected(void);
void log_set_selected(LogRec *);
LogRec *log_next_error(LogRec *);
LogRec *log_prev_error(LogRec *);

int log_get_flags(void);
void log_set_flags(int);
int log_num_errors(void);
int log_num_warnings(void);
char *log_annotations(void);

void log_save(const char *file, Progress *);
void log_open(const char *file, Progress *);
/* generates PostScript on given FILE* */
void log_print(FILE *fp);
/*
 * Filters line, converts it to zero or more LogRecs
 * and returns the last LogRec or NULL.
 */
LogRec *log_add_line(const char *line);
void log_ensure_visible(const LogRec *);

/* get the text displayed for the given logrec */
const char *log_get_text(const LogRec *lr);

/* Apply the given function to each line of the log
 * in turn. Stops prematurely if function returns FALSE.
 */
typedef gboolean (*LogApplyFunc)(LogRec *lr, gpointer user_data);
void log_apply(LogApplyFunc func, gpointer user_data);
void log_apply_after(LogApplyFunc func, gboolean forwards,
    LogRec *after, gpointer user_data);

/* Short term hack for colours sample in Preferences window */
void log_get_icon(LogSeverity level,
    GdkPixmap **open_pm, GdkBitmap **open_mask,
    GdkPixmap **closed_pm, GdkBitmap **closed_mask);

/*
 * Create a task whose output goes into the log.  Takes over
 * the new string `command'.  In main.c.
 */
Task *logged_task(char *command);

#endif /* _LOG_H_ */
