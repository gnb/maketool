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

#ifndef _MAKETOOL_H_
#define _MAKETOOL_H_

#include "common.h"
#include <gtk/gtk.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Structures.
 */
 
typedef struct
{
    char *name;
    char *value;
    enum { VAR_MAKE, VAR_ENVIRON } type;
} Variable;

/*
 * TODO
 *
 * typedef struct
 * {
 *     char *name;
 *     char *flags;
 * } BuildStyle;
 */
 
typedef enum
{
    LF_SHOW_INFO		=1<<0,
    LF_SHOW_WARNINGS		=1<<1,
    LF_SHOW_ERRORS		=1<<2,
    LF_SUMMARISE		=1<<3,
    LF_INDENT_DIRS		=1<<4,
    
    LF_DEFAULT_VALUE=		LF_SHOW_INFO|LF_SHOW_WARNINGS|LF_SHOW_ERRORS
} LogFlags;

typedef enum
{
    COL_BG_INFO,
    COL_FG_INFO,
    COL_BG_WARNING,
    COL_FG_WARNING,
    COL_BG_ERROR,
    COL_FG_ERROR,
    
    COL_MAX
} Colors;

typedef struct
{
    enum { RUN_SERIES, RUN_PARALLEL_PROC, RUN_PARALLEL_LOAD } run_how;
    int run_processes;
    int run_load;	/* load*10 */
    
    gboolean edit_first_error;
    gboolean edit_warnings;
    gboolean ignore_failures;
    
    enum { START_NOTHING, START_CLEAR, START_COLLAPSE } start_action;
    char *makefile;
    
    GList *variables;		/* list of Variable structs */
    char *var_make_flags;	/* Variables as make commandline N=V options */
    char **var_environment;	/* Variables as array of env "N=V" strings */
    
    char *prog_make;
    char *prog_list_targets;
    char *prog_list_version;
    char *prog_edit_source;
    char *prog_make_makefile;
    
    int win_width;		/* size of main window */
    int win_height;
    
    int log_flags;		/* bitwise OR of LF_* flags */
    
    gboolean dryrun;
    
    /* X11 color names, or (char*)0 for no setting */
    char *colors[COL_MAX];
} Preferences;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Global variable.
 */
 
#ifdef DEFINE_GLOBALS
#define EXTERN
#define VALUE(x) = x
#else
#define EXTERN extern
#define VALUE(x)
#endif

EXTERN const char *argv0;
EXTERN Preferences prefs;
EXTERN GtkWidget *toplevel;

#define SPACING 4

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Functions.
 */
 
/*main.c*/
void message(const char *fmt, ...);
void grey_menu_items(void);
char *expand_prog(const char *prog, const char *file, int line, const char *target);
/* help.c */ 
void help_about_cb(GtkWidget *, gpointer);
void help_about_make_cb(GtkWidget *, gpointer);
/* preferences.c */
void preferences_load(void);
void preferences_save(void);
void preferences_resize(int width, int height);
void preferences_set_dryrun(gboolean d);
void preferences_add_variable(const char *name, const char *value, int type);
void edit_preferences_cb(GtkWidget *, gpointer);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _MAKETOOL_H_ */
