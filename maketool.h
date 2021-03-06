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

#ifndef _MAKETOOL_H_
#define _MAKETOOL_H_

#include "common.h"
#include "maketool_task.h"
#include "ui.h"

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
    COL_BG_SUMMARY,
    COL_FG_SUMMARY,
    
    COL_MAX
} Colors;

typedef struct
{
    gboolean found_old;	    /* succeeded in loading old config database */
    gboolean upgraded;	    /* upgraded a value from old to new format */

    enum { RUN_SERIES, RUN_PARALLEL_PROC, RUN_PARALLEL_LOAD } run_how;
    int run_processes;
    int run_load;	/* load*10 */
    
    gboolean edit_first_error;
    gboolean edit_warnings;
    gboolean keep_going;
    gboolean enable_make_makefile;
    gboolean scroll_on_output;
    
    enum { START_NOTHING, START_CLEAR, START_COLLAPSE } start_action;
    enum { FINISH_NOTHING, FINISH_BEEP, FINISH_COMMAND, FINISH_DIALOG } finish_action;
    char *makefile;
    
    GList *variables;		/* list of Variable structs */
    char *var_make_flags;	/* Variables as make commandline N=V options */
    char **var_environment;	/* Variables as array of env "N=V" strings */

    char *makeprog;    
    char *prog_make;
    char *prog_list_targets;
    char *prog_list_version;
    char *prog_edit_source;
    char *prog_make_makefile;
    char *prog_help_browser;
    char *prog_finish;
    
    int win_width;		/* size of main window */
    int win_height;
    
    int log_flags;		/* bitwise OR of LF_* flags */
    
    gboolean dryrun;
    
    /* X11 color names, or (char*)0 for no setting */
    char *colors[COL_MAX];
    
    /* printing parameters */
    char *paper_name;
    int paper_width;	    /* in PS units (1/72 inch) */
    int paper_height;
    int margin_left;
    int margin_right;
    int margin_top;
    int margin_bottom;

#define MAX_DIR_HISTORY 16
    GList *dir_history;	    	/* list of strings */

} Preferences;

typedef struct
{
    const char *label;
    const char *command;
    void (*handler)(GtkWidget *, gpointer);
} MakeCommand;

typedef struct MakeSystem_s MakeSystem;

struct MakeSystem_s
{
    const char *name;
    const char *label;
    gboolean (*probe)(void);
    const MakeSystem *parent;	    /* points up inheritance hierarchy */
    gboolean automatic;     	    /* can do useful automatic update */
    char *makefile; 	    	    /* usually "Makefile" */
    const char * const *makefile_deps;     /* all filenames on which Makefile depends */
    const char * const *standard_targets;  /* standard targets, or NULL for GNU defaults */
    const MakeCommand *commands;    /* specific commands for Build menu */
};


typedef struct
{
    const char *name;
    const char *executable;
    const char *label;
    const char **logo_xpm;

    /*
     * Standard default filenames for the makefile in the same order
     * searched by the make program, e.g. GNUmakefile, makefile, Makefile.
     */
    const char * const *makefiles;
    /*
     * Construct one or more flags to implement prefs.makefile.
     */
    void (*makefile_flags)(estring *);
    /*
     * Construct one or more flags to implement the best approximation
     * available to the policy described by prefs.run_{how,processes,load}.
     */
    void (*parallel_flags)(estring *);
    /*
     * Construct one or more flags to implement prefs.ignore_failures.
     */
    void (*keep_going_flags)(estring *);
    /*
     * Construct one or more flags to implement prefs.dryrun.
     */
    void (*dryrun_flags)(estring *);
    /*
     * Construct one or more flags which emit version/copyright info
     * on stdout and/or stderr.
     */
    void (*version_flags)(estring *);
    /*
     * Construct one or more flags which emit a list of targets on
     * stdout and/or stderr.
     */
    void (*list_targets_flags)(estring *);
    /*
     * Return a new Task which expands and runs the given command,
     * (which will contain the list_targets flags) filtering the output
     * for targets and calling add_target() on each one.
     */
    Task *(*list_targets_task)(char *cmd);
    
} MakeProgram;


typedef enum
{
    GR_NONE=-1,
    
    GR_NOTEMPTY=0,
    GR_NOTRUNNING,
    GR_CLEAR_LOG,   	/* !running && !empty */
    GR_RUNNING,
    GR_SELECTED,
    GR_EDITABLE,
    GR_AGAIN,
    GR_ALL,
    GR_CLEAN,
    GR_FIND_AGAIN,
    GR_NEVER,	    	/* never active */
    GR_PRINT_PRINTER,
    GR_PRINT_FILE,

    NUM_SETS
} Groups;


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
void message_flush(void);
void grey_menu_items(void);
char *expand_prog(const char *prog, const char *file, int line, const char *target);
void handle_line(Task *task, int len, const char *line);
void set_targets(unsigned int ntargs, char **targs);
void set_makeprog(const char *name);
extern const MakeProgram *makeprog;
void list_targets_error(const char *errmsg);
gboolean filter_target(const char *targ);
const char *current_directory(void);
extern const MakeSystem *makesys;
extern const MakeProgram *makeprog;
/* print.c */ 
void lpr_init(void);
FILE *lpr_job_begin(const char *printer);
void lpr_job_end(FILE *fp);
void file_print_cb(GtkWidget *, gpointer);
/* help.c */ 
void help_about_cb(GtkWidget *, gpointer);
void help_about_make_cb(GtkWidget *, gpointer);
void help_goto_tag_cb(GtkWidget *w, gpointer data);
void help_goto_url_cb(GtkWidget *w, gpointer data);
void help_on_cb(GtkWidget *w, void *user_data);
/* find.c */
void edit_find_cb(GtkWidget *w, gpointer data);
void edit_find_again_cb(GtkWidget *w, gpointer data);
gboolean find_can_find_again(void);
/* preferences.c */
void preferences_load(void);
void preferences_save(void);
void preferences_resize(int width, int height);
void preferences_set_dryrun(gboolean d);
void preferences_add_variable(const char *name, const char *value, int type);
void edit_preferences_cb(GtkWidget *, gpointer);
void print_page_setup_cb(GtkWidget *, gpointer);
/* makesys.c */
gboolean ms_makefile_needs_update(const MakeSystem *ms);
gboolean ms_is_standard_target(const MakeSystem *ms, const char *targ);
const MakeSystem *ms_probe(void);
extern const MakeSystem * const makesystems[];
/* makeprog.c */
const char *mp_which_makefile(const MakeProgram *mp);
extern const MakeProgram * const makeprograms[];
const MakeProgram *mp_find(const char *name);
/* autoconf.c */
long show_configure_window(gboolean from_client);



#define g_list_find_str(l, s) \
	g_list_find_custom((l), (gpointer)(s), (GCompareFunc)strcmp)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _MAKETOOL_H_ */
