#ifndef _MAKETOOL_H_
#define _MAKETOOL_H_

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <glib.h>
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

typedef struct
{
    enum { RUN_SERIES, RUN_PARALLEL_PROC, RUN_PARALLEL_LOAD } run_how;
    int run_processes;
    float run_load;
    
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
void preferences_init(void);
void preferences_load(void);
void preferences_save(void);
void view_preferences_cb(GtkWidget *, gpointer);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _MAKETOOL_H_ */
