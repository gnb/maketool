/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2000 Greg Banks
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

#include "ui.h"
#include "maketool.h"
#include "util.h"
#include "log.h"

CVSID("$Id: preferences.c,v 1.44 2001-03-09 17:15:45 gnb Exp $");

static GtkWidget	*prefs_shell = 0;
static GtkWidget    	*notebook;
static GtkWidget	*run_proc_sb;
static GtkWidget	*run_load_sb;
static GtkWidget	*edit1_check;
static GtkWidget	*editw_check;
static GtkWidget	*fail_check;
static GtkWidget	*emm_check;
static GtkWidget	*soo_check;
static GtkWidget	*run_radio[3];
static GtkWidget	*prog_make_entry;
static GtkWidget	*prog_targets_entry;
static GtkWidget	*prog_version_entry;
static GtkWidget	*prog_edit_entry;
static GtkWidget	*prog_make_makefile_entry;
static GtkWidget	*prog_finish_entry;
static GtkWidget	*start_action_combo;
static GtkWidget	*finish_action_combo;
static GtkWidget	*makefile_entry;
static GtkWidget	*var_clist;
static GtkWidget	*var_name_entry;
static GtkWidget	*var_value_entry;
static GtkWidget	*var_type_combo;
static GtkWidget	*var_set_btn;
static GtkWidget	*var_unset_btn;
static gboolean		creating = TRUE, setting = FALSE;
static char		*var_type_strs[2];
static GtkWidget    	*color_dialog;
static GtkWidget    	*color_selector;
const char  	    	*color_labels[COL_MAX];
static GtkWidget	*color_entries[COL_MAX];
static int  	    	color_current_index = -1;
static gboolean     	color_from_selector;
static GtkWidget    	*color_sample_ctree;
static GtkCTreeNode 	*color_sample_node[L_MAX];
static int  	    	page_setup_index;
static GtkWidget    	*paper_name_combo;
static GtkWidget    	*paper_orientation_combo;
static GtkWidget    	*paper_width_entry;
static GtkWidget    	*paper_height_entry;
static GtkWidget    	*margin_left_entry;
static GtkWidget    	*margin_right_entry;
static GtkWidget    	*margin_top_entry;
static GtkWidget    	*margin_bottom_entry;

typedef enum
{
    VC_NAME, VC_VALUE, VC_TYPE, VC_MAX
} VarColumns;

static void update_color_sample(void);

#define DEFAULT_COL_BG_INFO	0
#define DEFAULT_COL_BG_WARNING  "#ffffc2"
#define DEFAULT_COL_BG_ERROR	"#ffc2c2"
#define DEFAULT_COL_FG_INFO	0
#define DEFAULT_COL_FG_WARNING  0
#define DEFAULT_COL_FG_ERROR	0


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
var_type_to_str(int type)
{
    return var_type_strs[type];
}

static int
var_str_to_type(const char *str)
{
    int i;
    
    for (i=0 ; i<ARRAYLEN(var_type_strs) ; i++)
	if (!strcmp(str, var_type_strs[i]))
    	    return i;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
preferences_add_variable(const char *name, const char *value, int type)
{
    Variable *var;
    
    var = g_new(Variable, 1);
    var->name = g_strdup(name);
    var->value = g_strdup(value);
    var->type = type;
    
    prefs.variables = g_list_append(prefs.variables, var);
}

#if 0
static Variable *
prefs_find_variable(const char *name)
{
    GList *list;
    
    for (list = prefs.variables ; list != 0 ; list = list->next)
    {
        Variable *var = (Variable *)list->data;
	
	if (!strcmp(var->name, name))
	    return var;
    }
    return 0;
}
#endif

static void
variable_delete(Variable *var)
{
    g_free(var->name);
    g_free(var->value);
    g_free(var);
}

#if 0
static void
prefs_remove_variable(const char *name)
{
    GList *list;
    
    for (list = prefs.variables ; list != 0 ; list = list->next)
    {
        Variable *var = (Variable *)list->data;
	
	if (!strcmp(var->name, name))
	{
	    variable_delete(var);
	    prefs.variables = g_list_remove_link(prefs.variables, list);
	    return;
	}
    }
}
#endif

static void
prefs_clear_variables(void)
{
    while (prefs.variables != 0)
    {
	variable_delete((Variable *)prefs.variables->data);
        prefs.variables = g_list_remove_link(prefs.variables, prefs.variables);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Update the two data structures most useful for
 * running make from the list of Variables.
 */

static void
prefs_set_var_environment(void)
{
    estring e;
    GList *list;
    int i;
    
    if (prefs.var_environment != 0)
    {
    	for (i=0 ; prefs.var_environment[i] != 0 ; i++)
	    g_free(prefs.var_environment[i]);
    	g_free(prefs.var_environment);
    }
    prefs.var_environment = g_new(char*, g_list_length(prefs.variables)+1);
    
    for (list = prefs.variables, i = 0 ; list != 0 ; list = list->next)
    {
    	Variable *var = (Variable *)list->data;
	
	if (var->type != VAR_ENVIRON)
	    continue;
	
	estring_init(&e);	/* data from previous it'n is in array */
	estring_append_string(&e, var->name);
	estring_append_char(&e, '=');
	estring_append_string(&e, var->value);
	prefs.var_environment[i++] = e.data;
    }
    prefs.var_environment[i++] = 0;
}


static void
prefs_set_var_make_flags(void)
{
    estring out;
    GList *list;
    static const char shell_metas[] = " \t\n\r\"'\\*;><?";
    
    estring_init(&out);
    for (list = prefs.variables ; list != 0 ; list = list->next)
    {
    	Variable *var = (Variable *)list->data;
	
	if (var->type != VAR_MAKE)
	    continue;
	
    	if (out.length > 0)
	    estring_append_char(&out, ' ');
	estring_append_string(&out, var->name);
	estring_append_char(&out, '=');
	if (strchr(var->value, '\''))
	{
	    const char *p;
	    for (p = var->value ; *p ; p++)
	    {
		if (strchr(shell_metas, *p))
		    estring_append_char(&out, '\\');
		estring_append_char(&out, *p);
	    }
	}
	else
	{
	    estring_append_char(&out, '\'');
	    estring_append_string(&out, var->value);
	    estring_append_char(&out, '\'');
	}
    }
    
    if (prefs.var_make_flags != 0)
    	g_free(prefs.var_make_flags);
    prefs.var_make_flags = out.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static UiEnumRec run_how_enum_def[] = {
{"RUN_SERIES",		RUN_SERIES},
{"RUN_PARALLEL_PROC",	RUN_PARALLEL_PROC},
{"RUN_PARALLEL_LOAD",	RUN_PARALLEL_LOAD},
{0, 0}};

static UiEnumRec start_action_enum_def[] = {
{"START_NOTHING",	START_NOTHING},
{"START_CLEAR",		START_CLEAR},
{"START_COLLAPSE",	START_COLLAPSE},
{0, 0}};

static UiEnumRec finish_action_enum_def[] = {
{"FINISH_NOTHING",	FINISH_NOTHING},
{"FINISH_BEEP",		FINISH_BEEP},
{"FINISH_COMMAND",	FINISH_COMMAND},
{"FINISH_DIALOG",	FINISH_DIALOG},
{0, 0}};

static UiEnumRec var_type_enum_def[] = {
{"VAR_MAKE",		VAR_MAKE},
{"VAR_ENVIRON",		VAR_ENVIRON},
{0, 0}};

static UiEnumRec log_flags_def[] = {
{"SHOW_INFO",    	LF_SHOW_INFO},
{"SHOW_WARNINGS",	LF_SHOW_WARNINGS},
{"SHOW_ERRORS",  	LF_SHOW_ERRORS},
{"SUMMARISE",    	LF_SUMMARISE},
{"INDENT_DIRS",    	LF_INDENT_DIRS},
{0, 0}};

void
preferences_load(void)
{
    int i;

    ui_config_init("maketool");
    prefs.run_how = ui_config_get_enum("run_how", RUN_SERIES, run_how_enum_def);
    prefs.run_processes = ui_config_get_int("run_processes", 2);
    prefs.run_load = ui_config_get_int("run_load", 20);
        
    prefs.edit_first_error = ui_config_get_boolean("edit_first_error", FALSE);
    prefs.edit_warnings = ui_config_get_boolean("edit_warnings", TRUE);
    prefs.ignore_failures = ui_config_get_boolean("ignore_failures", FALSE);
    prefs.enable_make_makefile = ui_config_get_boolean("enable_make_makefile", TRUE);
    prefs.scroll_on_output = ui_config_get_boolean("scroll_on_output", FALSE);
    
    prefs.start_action = ui_config_get_enum("start_action", START_COLLAPSE,
    				start_action_enum_def);
    prefs.finish_action = ui_config_get_enum("finish_action", FINISH_NOTHING,
    				finish_action_enum_def);
    prefs.makefile = ui_config_get_string("makefile", 0);
    
    prefs.variables = 0;
    /* TODO: load variables */
    prefs_set_var_environment();
    prefs_set_var_make_flags();
    
    prefs.prog_make = ui_config_get_string("prog_make", GMAKE " %n %m %k %p %v %t");
    prefs.prog_list_targets = ui_config_get_string("prog_list_targets", "extract_targets %m %v");
    prefs.prog_list_version = ui_config_get_string("prog_list_version", GMAKE " --version");
    prefs.prog_edit_source = ui_config_get_string("prog_edit_source", "nc -noask %{l:+-line %l} %f");
    prefs.prog_make_makefile = ui_config_get_string("prog_make_makefile", "make_makefile %m");
    prefs.prog_finish = ui_config_get_string("prog_finish", "");

    prefs.win_width = ui_config_get_int("win_width", 300);
    prefs.win_height = ui_config_get_int("win_height", 500);

    prefs.log_flags = ui_config_get_flags("log_flags", LF_DEFAULT_VALUE, log_flags_def);

    prefs.dryrun = ui_config_get_boolean("dryrun", FALSE);

    prefs.colors[COL_BG_INFO] = ui_config_get_string("bgcolor_info", DEFAULT_COL_BG_INFO);
    prefs.colors[COL_BG_WARNING] = ui_config_get_string("bgcolor_warning", DEFAULT_COL_BG_WARNING);
    prefs.colors[COL_BG_ERROR] = ui_config_get_string("bgcolor_error", DEFAULT_COL_BG_ERROR);
    prefs.colors[COL_FG_INFO] = ui_config_get_string("fgcolor_info", DEFAULT_COL_FG_INFO);
    prefs.colors[COL_FG_WARNING] = ui_config_get_string("fgcolor_warning", DEFAULT_COL_FG_WARNING);
    prefs.colors[COL_FG_ERROR] = ui_config_get_string("fgcolor_error", DEFAULT_COL_FG_ERROR);

    prefs.paper_name = ui_config_get_string("paper_name", "A4");
    prefs.paper_width = ui_config_get_int("paper_width", 595);
    prefs.paper_height = ui_config_get_int("paper_height", 842);
    prefs.margin_left = ui_config_get_int("margin_left", 36);
    prefs.margin_right = ui_config_get_int("margin_right", 36);
    prefs.margin_top = ui_config_get_int("margin_top", 72);
    prefs.margin_bottom = ui_config_get_int("margin_bottom", 72);


    /* Load directory history list from config file */
    i = 0;
    while (g_list_length(prefs.dir_history) < MAX_DIR_HISTORY)
    {
    	char *val, varname[20];
	
	g_snprintf(varname, sizeof(varname), "dir_history_%d", i);
	i++;
	if ((val = ui_config_get_string(varname, 0)) == 0)
	    break;
	/* ensure list is unique */
	if (g_list_find_str(prefs.dir_history, val) != 0)
	    continue;
	prefs.dir_history = g_list_append(prefs.dir_history, g_strdup(val));
    }
}

void
preferences_save(void)
{
    int i;
    GList *list;
    
    ui_config_set_enum("run_how", prefs.run_how, run_how_enum_def);
    ui_config_set_int("run_processes", prefs.run_processes);
    ui_config_set_int("run_load", prefs.run_load);
        
    ui_config_set_boolean("edit_first_error", prefs.edit_first_error);
    ui_config_set_boolean("edit_warnings", prefs.edit_warnings);
    ui_config_set_boolean("ignore_failures", prefs.ignore_failures);
    ui_config_set_boolean("enable_make_makefile", prefs.enable_make_makefile);
    ui_config_set_boolean("scroll_on_output", prefs.scroll_on_output);

    ui_config_set_enum("start_action", prefs.start_action,
    				start_action_enum_def);
    ui_config_set_enum("finish_action", prefs.finish_action,
    				finish_action_enum_def);
    ui_config_set_string("makefile", prefs.makefile);
    
    /* TODO: save variables */
    
    ui_config_set_string("prog_make", prefs.prog_make);
    ui_config_set_string("prog_list_targets", prefs.prog_list_targets);
    ui_config_set_string("prog_list_version", prefs.prog_list_version);
    ui_config_set_string("prog_edit_source", prefs.prog_edit_source);
    ui_config_set_string("prog_make_makefile", prefs.prog_make_makefile);
    ui_config_set_string("prog_finish", prefs.prog_finish);

    ui_config_set_int("win_width", prefs.win_width);
    ui_config_set_int("win_height", prefs.win_height);
    
    ui_config_set_flags("log_flags", prefs.log_flags, log_flags_def);

    ui_config_set_boolean("dryrun", prefs.dryrun);

    ui_config_set_string("bgcolor_info", prefs.colors[COL_BG_INFO]);
    ui_config_set_string("bgcolor_warning", prefs.colors[COL_BG_WARNING]);
    ui_config_set_string("bgcolor_error", prefs.colors[COL_BG_ERROR]);
    ui_config_set_string("fgcolor_info", prefs.colors[COL_FG_INFO]);
    ui_config_set_string("fgcolor_warning", prefs.colors[COL_FG_WARNING]);
    ui_config_set_string("fgcolor_error", prefs.colors[COL_FG_ERROR]);

    ui_config_set_string("paper_name", prefs.paper_name);
    ui_config_set_int("paper_width", prefs.paper_width);
    ui_config_set_int("paper_height", prefs.paper_height);
    ui_config_set_int("margin_left", prefs.margin_left);
    ui_config_set_int("margin_right", prefs.margin_right);
    ui_config_set_int("margin_top", prefs.margin_top);
    ui_config_set_int("margin_bottom", prefs.margin_bottom);
    
    
    for (list = prefs.dir_history, i = 0 ; list != 0 ; list = list->next, i++)
    {
    	char *dir = (char *)list->data;
    	char varname[20];
	
	g_snprintf(varname, sizeof(varname), "dir_history_%d", i);
	ui_config_set_string(varname, dir);
    }

    ui_config_sync();
}

void
preferences_resize(int width, int height)
{
    prefs.win_width = width;
    prefs.win_height = height;
    ui_config_set_int("win_width", prefs.win_width);
    ui_config_set_int("win_height", prefs.win_height);
    ui_config_sync();
}

void
preferences_set_dryrun(gboolean d)
{
    prefs.dryrun = d;
    ui_config_set_boolean("dryrun", prefs.dryrun);
    ui_config_sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
prefs_apply_cb(GtkWidget *w, gpointer data)
{
    char *mf;
    char *text[VC_MAX];
    int row, nrows, i;
    gboolean colors_changed;
    
#if DEBUG
    fprintf(stderr, "prefs_apply_cb()\n");
#endif
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run_radio[RUN_PARALLEL_PROC])))
    	prefs.run_how = RUN_PARALLEL_PROC;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run_radio[RUN_PARALLEL_LOAD])))
    	prefs.run_how = RUN_PARALLEL_LOAD;
    else
    	prefs.run_how = RUN_SERIES;

        
    prefs.run_processes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(run_proc_sb));
    prefs.run_load = (int)(10.0 * gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(run_load_sb)) + 0.5);
#if DEBUG > 4
    fprintf(stderr, "run_load = (double)%g = (int)%d\n",
    	10.0*(double)gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(run_load_sb)),
	prefs.run_load);
#endif
    
    prefs.edit_first_error = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit1_check));
    prefs.edit_warnings = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editw_check));
    prefs.ignore_failures = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fail_check));
    prefs.enable_make_makefile = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(emm_check));
    prefs.scroll_on_output = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(soo_check));

    g_free(prefs.prog_make);
    prefs.prog_make = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_make_entry)));
        
    g_free(prefs.prog_list_targets);
    prefs.prog_list_targets = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_targets_entry)));
    
    g_free(prefs.prog_list_version);
    prefs.prog_list_version = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_version_entry)));
    
    g_free(prefs.prog_edit_source);
    prefs.prog_edit_source = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_edit_entry)));
    
    g_free(prefs.prog_make_makefile);
    prefs.prog_make_makefile = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_make_makefile_entry)));
    
    g_free(prefs.prog_finish);
    prefs.prog_finish = g_strdup(gtk_entry_get_text(GTK_ENTRY(prog_finish_entry)));
    
    prefs.start_action = ui_combo_get_current(start_action_combo);
    prefs.finish_action = ui_combo_get_current(finish_action_combo);
    
    /* TODO: strip `mf' of whitespace JIC */
    mf = gtk_entry_get_text(GTK_ENTRY(makefile_entry));
    if (prefs.makefile != 0)
	g_free(prefs.makefile);
    prefs.makefile = (mf == 0 || *mf == '\0' ? 0 : g_strdup(mf));
    
    prefs_clear_variables();
    nrows = GTK_CLIST(var_clist)->rows;        
    for (row = 0 ; row < nrows ; row++)
    {
    	ui_clist_get_strings(var_clist, row, VC_MAX, text);
	preferences_add_variable(text[VC_NAME], text[VC_VALUE],
		var_str_to_type(text[VC_TYPE]));
    }
    prefs_set_var_environment();
    prefs_set_var_make_flags();

    colors_changed = FALSE;    
    for (i = 0 ; i < COL_MAX ; i++)
    {
    	const char *newval = gtk_entry_get_text(GTK_ENTRY(color_entries[i]));
	
	if (newval != 0 && *newval == '\0')
	    newval = 0;
	if (strcmp(safe_str(newval), safe_str(prefs.colors[i])))
	{
    	    if (prefs.colors[i] != 0)
		g_free(prefs.colors[i]);
	    prefs.colors[i] = g_strdup(newval);
	    colors_changed = TRUE;
	}
    }
    if (colors_changed)
    	log_colors_changed();
      
    preferences_save();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
changed_cb(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	ui_dialog_changed(prefs_shell);
	gtk_widget_set_sensitive(prog_finish_entry,
	    (ui_combo_get_current(finish_action_combo) == FINISH_COMMAND));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
set_makefile(const char *mf)
{
    gtk_entry_set_text(GTK_ENTRY(makefile_entry), mf);
    ui_dialog_changed(prefs_shell);
}

static void
browse_makefile_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    char *mf = gtk_entry_get_text(GTK_ENTRY(makefile_entry));
    
    if (filesel == 0)
    	filesel = ui_create_file_sel(
	    toplevel,
	    _("Choose Makefile"),
	    set_makefile,
	    mf);
    else
    	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), mf);

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static GtkWidget *
prefs_create_general_page(GtkWidget *toplevel)
{
    GtkWidget *table, *table2;
    GtkWidget *frame;
    GtkObject *adj;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *button;
    GtkWidget *entry;
    GList *list;
    int row = 0;
    
    table = gtk_table_new(8, 3, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    ui_set_help_name(table, "prefs-general-page");
    gtk_widget_show(table);

    
    /*
     * `Parallel' frame
     */
    frame = gtk_frame_new(_("Make runs commands"));
    gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 3, row, row+1);
    gtk_widget_show(frame);
    
    table2 = gtk_table_new(3, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table2), SPACING);
    gtk_container_add(GTK_CONTAINER(frame), table2);
    gtk_widget_show(table2);
    
    run_radio[RUN_SERIES] = gtk_radio_button_new_with_label_from_widget(0,
    			_("in series."));
    gtk_signal_connect(GTK_OBJECT(run_radio[RUN_SERIES]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_radio[RUN_SERIES], 0, 2, 0, 1);
    ui_set_help_name(run_radio[RUN_SERIES], "prefs-run-series");
    gtk_widget_show(run_radio[RUN_SERIES]);
    

    run_radio[RUN_PARALLEL_PROC] = gtk_radio_button_new_with_label_from_widget(
    			GTK_RADIO_BUTTON(run_radio[RUN_SERIES]),
    			_("in parallel. Maximum number of processes is "));
    gtk_signal_connect(GTK_OBJECT(run_radio[RUN_PARALLEL_PROC]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_radio[RUN_PARALLEL_PROC], 0, 1, 1, 2);
    ui_set_help_name(run_radio[RUN_PARALLEL_PROC], "prefs-run-proc");
    gtk_widget_show(run_radio[RUN_PARALLEL_PROC]);

    adj = gtk_adjustment_new(
    		(gfloat)2, 	/* value */
                (gfloat)2,	/* lower */
                (gfloat)10,	/* upper */
                (gfloat)1,	/* step_increment */
                (gfloat)5,	/* page_increment */
                (gfloat)0);	/* page_size */
    run_proc_sb = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.5, 0);		
    gtk_signal_connect(GTK_OBJECT(run_proc_sb), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_proc_sb, 1, 2, 1, 2);
    gtk_widget_show(run_proc_sb);

    
    
    run_radio[RUN_PARALLEL_LOAD] = gtk_radio_button_new_with_label_from_widget(
    			GTK_RADIO_BUTTON(run_radio[RUN_SERIES]),
    			_("in parallel. Maximum machine load is "));
    gtk_signal_connect(GTK_OBJECT(run_radio[RUN_PARALLEL_LOAD]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_radio[RUN_PARALLEL_LOAD], 0, 1, 2, 3);
    ui_set_help_name(run_radio[RUN_PARALLEL_LOAD], "prefs-run-load");
    gtk_widget_show(run_radio[RUN_PARALLEL_LOAD]);
    
    adj = gtk_adjustment_new(
    		(gfloat)2.0, 	/* value */
                (gfloat)0.1,	/* lower */
                (gfloat)5.0,	/* upper */
                (gfloat)0.1,	/* step_increment */
                (gfloat)0.5,	/* page_increment */
                (gfloat)0);	/* page_size */
    run_load_sb = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.5, 1);		
    gtk_signal_connect(GTK_OBJECT(run_load_sb), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_load_sb, 1, 2, 2, 3);
    gtk_widget_show(run_load_sb);
    row++;

    /*
     * Edit first detected error.
     */
    edit1_check = gtk_check_button_new_with_label(_("Automatically edit first error"));
    gtk_signal_connect(GTK_OBJECT(edit1_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), edit1_check, 0, 3, row, row+1);
    ui_set_help_name(edit1_check, "prefs-auto-edit");
    gtk_widget_show(edit1_check);
    row++;
    
    /*
     * Auto edit warnings
     */
    editw_check = gtk_check_button_new_with_label(_("Edit Next ignores warnings"));
    gtk_signal_connect(GTK_OBJECT(editw_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), editw_check, 0, 3, row, row+1);
    gtk_widget_show(editw_check);
    ui_set_help_name(editw_check, "prefs-edit-warnings");
    row++;
    
    /*
     * -k flag
     */
    fail_check = gtk_check_button_new_with_label(_("Continue despite failures"));
    gtk_signal_connect(GTK_OBJECT(fail_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), fail_check, 0, 3, row, row+1);
    ui_set_help_name(fail_check, "prefs-kflag");
    gtk_widget_show(fail_check);
    row++;

    /*
     * enable make_makefiles script
     */
    emm_check = gtk_check_button_new_with_label(_("Build Makefile from Imakefile or Makefile.in"));
    gtk_signal_connect(GTK_OBJECT(emm_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), emm_check, 0, 3, row, row+1);
    gtk_widget_show(emm_check);
    ui_set_help_name(emm_check, "prefs-enable-mmf");
    row++;

    /*
     * scroll to end of window on make child's output
     */
    soo_check = gtk_check_button_new_with_label(_("Scroll to end on output"));
    gtk_signal_connect(GTK_OBJECT(soo_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), soo_check, 0, 3, row, row+1);
    gtk_widget_show(soo_check);
    ui_set_help_name(soo_check, "prefs-scroll-output");
    row++;

    /*
     * Action on starting build.
     */
    label = gtk_label_new(_("On starting each build: "));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("Do nothing"));
    list = g_list_append(list, _("Clear log"));
    list = g_list_append(list, _("Collapse all log items"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, row, row+1);
    gtk_widget_show(combo);
    ui_set_help_name(combo, "prefs-start-action");
    start_action_combo = combo;
    row++;

    /*
     * Action on finishing build.
     */
    label = gtk_label_new(_("On finishing each build: "));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("Do nothing"));
    list = g_list_append(list, _("Beep"));
    list = g_list_append(list, _("Run command"));
    list = g_list_append(list, _("Show dialog"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, row, row+1);
    gtk_widget_show(combo);
    ui_set_help_name(combo, "prefs-finish-action");
    finish_action_combo = combo;
    row++;

    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 3, row, row+1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_widget_show(entry);
    prog_finish_entry = entry;
    row++;


    /*
     * Choose Makefile.
     */
    
    label = gtk_label_new(_("Makefile:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("(default)"));
    list = g_list_append(list, "Makefile");
    list = g_list_append(list, "makefile");
    list = g_list_append(list, "GNUmakefile");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_combo_set_item_string(GTK_COMBO(combo),    
    		GTK_ITEM(gtk_container_children(
			GTK_CONTAINER(GTK_COMBO(combo)->list))->data),
		"");
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_widget_show(combo);
    ui_set_help_name(combo, "prefs-fflag");
    makefile_entry = GTK_COMBO(combo)->entry;

    button = gtk_button_new_with_label(_("Browse..."));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(browse_makefile_cb), 0);
    gtk_table_attach(GTK_TABLE(table), button, 2, 3, row, row+1,
		       (GtkAttachOptions)0, (GtkAttachOptions)0,
		       SPACING, 0);
    ui_set_help_name(button, "prefs-fflag");
    gtk_widget_show(button);

    row++;

    return table;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
var_select_cb(
    GtkCList *clist,
    gint row,
    gint col,
    GdkEvent *event, 
    gpointer data)
{
    char *text[VC_MAX];

    ui_clist_get_strings(var_clist, row, VC_MAX, text);

    setting = TRUE;	/* don't ungrey the Apply button */
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), text[VC_NAME]);
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), text[VC_VALUE]);
    ui_combo_set_current(var_type_combo, var_str_to_type(text[VC_TYPE]));
    setting = FALSE;
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, TRUE);
}


static void
var_handle_equal(void)
{
    const char *namex;
    const char *valuex;
    char *name;
    char *value;
    
    namex = gtk_entry_get_text(GTK_ENTRY(var_name_entry));
    if (namex == 0 || strchr(namex, '=') == 0)
    	return;
    namex = safe_str(namex);
    
    valuex = gtk_entry_get_text(GTK_ENTRY(var_value_entry));
    valuex = safe_str(valuex);

    name = g_strconcat(namex, valuex, 0);
    value = strchr(name, '=');
    *value++ = '\0';
    
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), name);
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), value);
    gtk_widget_grab_focus(var_value_entry);
    
    g_free(name);
}

static void
var_changed_cb(GtkWidget *w, gpointer data)
{
    if (!creating && !setting)
    {
	gtk_widget_set_sensitive(var_set_btn, TRUE);
	gtk_widget_set_sensitive(var_unset_btn, TRUE);
	
	if (w == var_name_entry)
	    var_handle_equal();
    }
}

static void
var_set_cb(GtkWidget *w, gpointer data)
{
    int row;
    char *text[VC_MAX];
    char *rtext[VC_MAX];
    int nrows = GTK_CLIST(var_clist)->rows;
    
    text[VC_NAME] = gtk_entry_get_text(GTK_ENTRY(var_name_entry));
    text[VC_VALUE] = gtk_entry_get_text(GTK_ENTRY(var_value_entry));
    text[VC_TYPE] = var_type_to_str(ui_combo_get_current(var_type_combo));
        
    for (row = 0 ; row < nrows ; row++)
    {
    	ui_clist_get_strings(var_clist, row, VC_MAX, rtext);
	if (!strcmp(rtext[VC_NAME], text[VC_NAME]))
	{
	    if (!strcmp(text[VC_VALUE], rtext[VC_VALUE]) &&
	        !strcmp(text[VC_TYPE], rtext[VC_TYPE]))
	    	return;	/* same variable, no change to value or type: ignore */
    	    /* update list with new value */
    	    ui_clist_set_strings(var_clist, row, VC_MAX, text);
	    break;
	}
    }
    
    if (row == nrows)
    {
    	/* no existing row with this variable */
	gtk_clist_append(GTK_CLIST(var_clist), text);
    }
    
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, TRUE);
    ui_dialog_changed(prefs_shell);
}

static void
var_unset_cb(GtkWidget *w, gpointer data)
{
    int row;
    char *name;
    char *rname;
    gboolean changed = FALSE;
    int nrows = GTK_CLIST(var_clist)->rows;
    
    name = gtk_entry_get_text(GTK_ENTRY(var_name_entry));
        
    for (row = 0 ; row < nrows ; row++)
    {
	gtk_clist_get_text(GTK_CLIST(var_clist), row, VC_NAME, &rname);
	if (!strcmp(rname, name))
	{
	    gtk_clist_remove(GTK_CLIST(var_clist), row);
	    changed = TRUE;
	    break;
	}
    }

    setting = TRUE; 	/* don't ungrey the Apply button */
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), "");
    ui_combo_set_current(var_type_combo, 0);
    setting = FALSE;
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, FALSE);
    if (changed)
	ui_dialog_changed(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static GtkWidget *
prefs_create_variables_page(GtkWidget *toplevel)
{
    GtkWidget *vbox;
    GtkWidget *clist;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *combo;
    GtkWidget *sb;
    char *text[VC_MAX];
    int row = 0;
    GList *list;

    
    var_type_strs[VAR_MAKE] = _("Make");
    var_type_strs[VAR_ENVIRON] = _("Environ");

    vbox = gtk_vbox_new(FALSE, SPACING);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);
    ui_set_help_name(vbox, "prefs-variables-page");
    gtk_widget_show(vbox);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    text[VC_NAME] = _("Name");
    text[VC_VALUE] = _("Value");
    text[VC_TYPE] = _("Type");
    clist = gtk_clist_new_with_titles(VC_MAX, text);
    gtk_clist_set_sort_column(GTK_CLIST(clist), VC_NAME);
    gtk_clist_set_auto_sort(GTK_CLIST(clist), TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), VC_NAME, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), VC_VALUE, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), VC_TYPE, TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), clist, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(clist), "select_row", 
    	GTK_SIGNAL_FUNC(var_select_cb), 0);
    gtk_widget_show(clist);
    var_clist = clist;
    
    sb = gtk_vscrollbar_new(GTK_CLIST(clist)->vadjustment);
    gtk_box_pack_start(GTK_BOX(hbox), sb, FALSE, FALSE, 0);
    gtk_widget_show(sb);
    
    
    table = gtk_table_new(3, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    
    /*
     * Name
     */
    label = gtk_label_new(_("Name:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    var_name_entry = entry;
    
    row++;
    
    /*
     * Value
     */
    label = gtk_label_new(_("Value:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    var_value_entry = entry;
    
    row++;
    
    /*
     * Type
     */
    label = gtk_label_new(_("Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, var_type_strs[0]);
    list = g_list_append(list, var_type_strs[1]);
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(combo);
    var_type_combo = combo;
    
    row++;
    

    /*
     * Set & Unset buttons
     */
    hbox = gtk_hbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);
    
    button = gtk_button_new_with_label(_("Set"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(var_set_cb), 0);
    gtk_widget_set_sensitive(button, FALSE);
    gtk_widget_show(button);
    var_set_btn = button;

    button = gtk_button_new_with_label(_("Unset"));
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(var_unset_cb), 0);
    gtk_widget_set_sensitive(button, FALSE);
    gtk_widget_show(button);
    var_unset_btn = button;

    return vbox;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char programs_key[] = N_("\
Key\n\
%f              source filename\n\
%l              source line number\n\
%m              -f makefile if specified\n\
%n              dryrun flag (-n) if specified\n\
%p              parallel flags (-j or -l)\n\
%k              continue flag (-k) if specified\n\
%v              make variable overrides\n\
%t              make target\n\
%{x:+y}         y if %x is not empty\n\
");



static GtkWidget *
prefs_create_programs_page(GtkWidget *toplevel)
{
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *combo;
    GList *list;
    int row = 0;
    
    table = gtk_table_new(6, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    ui_set_help_name(table, "prefs-programs-page");
    gtk_widget_show(table);
    
    /*
     * Make program
     */
    label = gtk_label_new(_("Make:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    prog_make_entry = entry;
    
    row++;
    
    /*
     * Make target lister
     */
    label = gtk_label_new(_("List make targets:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    prog_targets_entry = entry;    
    row++;

    /*
     * Make version lister
     */
    label = gtk_label_new(_("List make version:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    prog_version_entry = entry;
    row++;

    /*
     * Editor
     */
    label = gtk_label_new(_("Edit source files:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, "gnome-edit %{l:++%l} %f");
    list = g_list_append(list, "gnome-terminal -e \"${VISUAL:-vi} %{l:++%l} %f\"");
    list = g_list_append(list, "xterm -e ${VISUAL:-vi} %{l:++%l} %f");
    list = g_list_append(list, "nc -noask %{l:+-line %l} %f");
    list = g_list_append(list, "emacsclient %{l:++%l} %f");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    entry = GTK_COMBO(combo)->entry;
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_widget_show(combo);
    prog_edit_entry = entry;
    row++;
    
    /*
     * Makefile updater
     */
    label = gtk_label_new(_("Build makefile:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    prog_make_makefile_entry = entry;
    row++;

    /*
     * Key
     */
    label = gtk_label_new(_(programs_key));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, row, row+1);
    gtk_widget_show(label);
    row++;
    
    return table;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Set the color in the color editing dialog.
 * First convert the X11 color name into 3 doubles,
 * which is what the color selector wants.
 */
static void
color_dialog_set_color(const char *color_name)
{
    GdkColor col;
    double dcol[3];

    if (color_name == 0 || !gdk_color_parse(color_name, &col))
    {
    	col.red = 0;
	col.green = 0;
	col.blue = 0;
    }
    dcol[0] = (double)col.red / 65535.0;
    dcol[1] = (double)col.green / 65535.0;
    dcol[2] = (double)col.blue / 65535.0;
    gtk_color_selection_set_color(GTK_COLOR_SELECTION(color_selector), dcol);
}



static void
color_entry_changed_cb(GtkWidget *w, gpointer data)
{
    int index = (int)data;
    
    if (creating)
    	return;

    if (!color_from_selector && index == color_current_index)
    {
        /* user typed or pasted new value directly into entry */
	color_dialog_set_color(gtk_entry_get_text(GTK_ENTRY(w)));
    }
    	
    ui_dialog_changed(prefs_shell);
    update_color_sample();
}

static void
color_selection_change_cb(GtkWidget *w, gpointer data)
{
    char name[14];
    double dcol[3];

    gtk_color_selection_get_color(GTK_COLOR_SELECTION(w), dcol);
    sprintf(name, "#%04X%04X%04X",
    	(int)(dcol[0] * 65535.0),
	(int)(dcol[1] * 65535.0),
	(int)(dcol[2] * 65535.0));
    color_from_selector = TRUE;
    gtk_entry_set_text(GTK_ENTRY(color_entries[color_current_index]), name);
    gtk_entry_select_region(GTK_ENTRY(color_entries[color_current_index]), 0, -1);
    color_from_selector = FALSE;
    update_color_sample();
}

static void
color_choose_cb(GtkWidget *w, gpointer data)
{
    char *title;
    
    /*
     * Create the color editing dialog if it doesn't exist.
     */
    if (color_dialog == 0)
    {
    	color_dialog = ui_create_ok_dialog(toplevel, "");
	ui_autonull_pointer(&color_dialog);
    	color_selector = gtk_color_selection_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(color_dialog)->vbox),
	    color_selector);
	gtk_signal_connect(GTK_OBJECT(color_selector), "color_changed", 
    	    GTK_SIGNAL_FUNC(color_selection_change_cb), 0);
	gtk_window_set_modal(GTK_WINDOW(color_dialog), TRUE);
	gtk_widget_show(GTK_WIDGET(color_selector));
    }
	
    /*
     * Setup the color editing dialog for the current color index.
     */
    if (color_current_index >= 0)
	gtk_entry_select_region(GTK_ENTRY(color_entries[color_current_index]), 0, 0);

    color_current_index = (int)data;

    title = g_strdup_printf(_("Maketool: %s Choose Color"),
    	color_labels[color_current_index]);
    gtk_window_set_title(GTK_WINDOW(color_dialog), title);
    g_free(title);
    
    color_dialog_set_color(gtk_entry_get_text(GTK_ENTRY(color_entries[color_current_index])));
    
    /*
     * Select the value in the entry widget corresponding to
     * the current color index. This gives the user additional
     * feedback about which color is being edited.
     */
    gtk_entry_select_region(GTK_ENTRY(color_entries[color_current_index]), 0, -1);

    gtk_widget_show(color_dialog);
}


static GtkWidget *
prefs_create_color_row(
    GtkWidget *table,
    int row,
    int color_num)
{
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *button;
    
    label = gtk_label_new(color_labels[color_num]);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1,
		       (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0,
		       0, 0);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(color_entry_changed_cb), (gpointer)color_num);
    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row+1,
		       (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0,
		       0, 0);
    gtk_widget_show(entry);
    
    button = gtk_button_new_with_label(_("Choose..."));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(color_choose_cb), (gpointer)color_num);
    gtk_table_attach(GTK_TABLE(table), button, 2, 3, row, row+1,
		       (GtkAttachOptions)0, (GtkAttachOptions)0, SPACING, 0);
    gtk_widget_show(button);

    /* 
     * Set current value. This is isolated at the bottom
     * of this function in case it needs to be separated later.
     */
    gtk_entry_set_text(GTK_ENTRY(entry), safe_str(prefs.colors[color_num]));
    
    return entry;
}

static void
set_sample_colors(GtkCTreeNode *node, const char *bg, const char *fg)
{
    GdkColor col;
    
    gtk_ctree_node_set_foreground(GTK_CTREE(color_sample_ctree), node,
    	(fg != 0 && gdk_color_parse(fg, &col) ? &col : 0));
    gtk_ctree_node_set_background(GTK_CTREE(color_sample_ctree), node,
	(bg != 0 && gdk_color_parse(bg, &col) ? &col : 0));
}

static void
update_color_sample(void)
{
    set_sample_colors(color_sample_node[L_INFO],
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_BG_INFO])),
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_FG_INFO])));
    set_sample_colors(color_sample_node[L_WARNING],
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_BG_WARNING])),
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_FG_WARNING])));
    set_sample_colors(color_sample_node[L_ERROR],
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_BG_ERROR])),
    	gtk_entry_get_text(GTK_ENTRY(color_entries[COL_FG_ERROR])));
}

static void
color_reset_old_cb(GtkWidget *w, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_INFO]), safe_str(prefs.colors[COL_BG_INFO]));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_INFO]), safe_str(prefs.colors[COL_FG_INFO]));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_WARNING]), safe_str(prefs.colors[COL_BG_WARNING]));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_WARNING]), safe_str(prefs.colors[COL_FG_WARNING]));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_ERROR]), safe_str(prefs.colors[COL_BG_ERROR]));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_ERROR]), safe_str(prefs.colors[COL_FG_ERROR]));
    /* update_color_sample(); */
}

static void
color_reset_defaults_cb(GtkWidget *w, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_INFO]), safe_str(DEFAULT_COL_BG_INFO));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_INFO]), safe_str(DEFAULT_COL_FG_INFO));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_WARNING]), safe_str(DEFAULT_COL_BG_WARNING));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_WARNING]), safe_str(DEFAULT_COL_FG_WARNING));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_BG_ERROR]), safe_str(DEFAULT_COL_BG_ERROR));
    gtk_entry_set_text(GTK_ENTRY(color_entries[COL_FG_ERROR]), safe_str(DEFAULT_COL_FG_ERROR));
    /* update_color_sample(); */
}

static GtkWidget *
prefs_create_colors_page(GtkWidget *toplevel)
{
    GtkWidget *table;
    GtkWidget *ctree, *frame;
    GtkWidget *hbox, *button;
    int row = 0, i;
    static char *titles[1] = { "Log" };
    
    table = gtk_table_new(8, 3, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    ui_set_help_name(table, "prefs-colors-page");
    gtk_widget_show(table);
    
    color_labels[COL_BG_INFO] = _("Information Background:");
    color_labels[COL_FG_INFO] = _("Information Foreground:");
    color_labels[COL_BG_WARNING] = _("Warning Background:");
    color_labels[COL_FG_WARNING] = _("Warning Foreground:");
    color_labels[COL_BG_ERROR] = _("Error Background:");
    color_labels[COL_FG_ERROR] = _("Error Foreground:");
    
    for (i = 0 ; i < COL_MAX ; i++)
	color_entries[i] = prefs_create_color_row(table, row++, i);
	
    frame = gtk_frame_new(_("Sample"));
    gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 3, row, row+1);
    gtk_widget_show(frame);

    ctree = gtk_ctree_new_with_titles(1, 0, titles);
    gtk_ctree_set_line_style(GTK_CTREE(ctree), GTK_CTREE_LINES_NONE);
    gtk_clist_column_titles_hide(GTK_CLIST(ctree));
    gtk_ctree_set_show_stub(GTK_CTREE(ctree), FALSE);
    gtk_clist_set_column_width(GTK_CLIST(ctree), 0, 400);
    gtk_clist_set_column_auto_resize(GTK_CLIST(ctree), 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(ctree), SPACING);
    gtk_container_add(GTK_CONTAINER(frame), ctree);
    for (i=0 ; i<L_MAX ; i++)
    {
    	char *text = 0;
	GdkPixmap *open_pm = 0, *closed_pm = 0;
	GdkBitmap *open_mask = 0, *closed_mask = 0;
    	switch (i)
	{
	case L_INFO:
	    text = _("information: yadda yadda yadda");
	    break;
	case L_WARNING:
	    text = _("warning: danger, Will Robinson!");
	    break;
	case L_ERROR:
	    text = _("error: I'm sorry Dave I can't do that");
	    break;
	default:
	    continue;
	}
	log_get_icon((LogSeverity)i,
	    &open_pm, &open_mask,
	    &closed_pm, &closed_mask);
	color_sample_node[i] = gtk_ctree_insert_node(GTK_CTREE(ctree),
    	    (GtkCTreeNode *)0,			/* parent */
	    (GtkCTreeNode*)0,			/* sibling */
	    &text,				/* text[] */
	    0,					/* spacing */
	    closed_pm,
	    closed_mask,  	    	    	/* pixmap_closed,mask_closed */
	    open_pm,
	    open_mask,		    	    	/* pixmap_opened,mask_opened */
	    TRUE,			    	/* is_leaf */
	    TRUE);			    	/* expanded */
    	gtk_ctree_node_set_selectable(GTK_CTREE(ctree), color_sample_node[i],
	    	    	    	    	FALSE);
    }
    gtk_widget_show(ctree);
    color_sample_ctree = ctree;
    row++;
    
    hbox = gtk_hbox_new(FALSE, SPACING);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 3, row, row+1,
		       (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0,
		       0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
    gtk_widget_show(hbox);
    
    button = gtk_button_new_with_label(_("Reset to Old"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(color_reset_old_cb), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    
    button = gtk_button_new_with_label(_("Reset to Defaults"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(color_reset_defaults_cb), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    
    row++;
    

    /* Set the colours of the sample tree widget. Separable. */
    update_color_sample();
    
    return table;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define MAXLENGTH 7

static const char units_name[] = N_("mm");
#define UNITS_FROM_PSUNITS(x)	    ((x)*25.4/72.0)

static GtkWidget *
prefs_create_units_label(void)
{
    GtkWidget *label;
    
    label = gtk_label_new(_(units_name));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    return label;
}

static void
prefs_set_units_entry(GtkWidget *entry, int n)
{
    char unitbuf[256];

    sprintf(unitbuf, "%d", (int)(UNITS_FROM_PSUNITS(n)+0.5));
    gtk_entry_set_text(GTK_ENTRY(entry), unitbuf);
}

static GtkWidget *
prefs_create_page_setup_page(GtkWidget *toplevel)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *entry;
    GList *list;
    int col, row;
    
    hbox = gtk_hbox_new(FALSE, SPACING);

    label = gtk_label_new(_("Large\ncanvas\nsample\ngoes\nhere"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_end(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    ui_set_help_name(hbox, "prefs-page=setup-page");
    gtk_widget_show(label);
    
    vbox = gtk_vbox_new(FALSE, SPACING);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);
    gtk_box_pack_end(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show(vbox);
    
    frame = gtk_frame_new(_("Paper Size"));
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    
    table = gtk_table_new(4, 3, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), SPACING);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_widget_show(table);
    
    row = 0;
    
    label = gtk_label_new(_("Name:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("A4"));
    list = g_list_append(list, _("B5"));
    list = g_list_append(list, _("US letter"));
    list = g_list_append(list, _("US legal"));
    list = g_list_append(list, _("Custom"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, row, row+1);
    gtk_widget_show(combo);
    paper_name_combo = combo;
    
    row++;
    
    label = gtk_label_new(_("Orientation:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("Portrait"));
    list = g_list_append(list, _("Landscape"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, row, row+1);
    gtk_widget_show(combo);
    paper_orientation_combo = combo;
    
    row++;
    
    label = gtk_label_new(_("Width:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    paper_width_entry = entry;
    
    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;
    
    label = gtk_label_new(_("Height:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    paper_height_entry = entry;
    
    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;
    

    frame = gtk_frame_new(_("Margins"));
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);
    
    table = gtk_table_new(4, 3, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), SPACING);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_widget_show(table);

    row = 0;
    col = 0;

    label = gtk_label_new(_("Left:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, col, col+1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new_with_max_length(MAXLENGTH);
    gtk_table_attach(GTK_TABLE(table), entry, col+1, col+2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    margin_left_entry = entry;

    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;

    label = gtk_label_new(_("Right:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, col, col+1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new_with_max_length(MAXLENGTH);
    gtk_table_attach(GTK_TABLE(table), entry, col+1, col+2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    margin_right_entry = entry;

    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;

    label = gtk_label_new(_("Top:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, col, col+1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new_with_max_length(MAXLENGTH);
    gtk_table_attach(GTK_TABLE(table), entry, col+1, col+2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    margin_top_entry = entry;

    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;

    label = gtk_label_new(_("Bottom:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, col, col+1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new_with_max_length(MAXLENGTH);
    gtk_table_attach(GTK_TABLE(table), entry, col+1, col+2, row, row+1,
    	(GtkAttachOptions)0,
	(GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
	0, 0);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    margin_bottom_entry = entry;

    label = prefs_create_units_label();
    gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, row, row+1);
    gtk_widget_show(label);
    
    row++;

    return hbox;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static GtkWidget *
prefs_create_styles_page(GtkWidget *toplevel)
{
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *entry;
    int row = 0;
    
    table = gtk_table_new(1, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_widget_show(table);
    
    
    label = gtk_label_new(_("Editor:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "...styles...");
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    return table;
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Show the current preferences values in the GUI widgets
 */

static void
prefs_show(void)
{
    GList *list;

    color_reset_old_cb(0, 0);

    gtk_toggle_button_set_active(
    	GTK_TOGGLE_BUTTON(run_radio[prefs.run_how]), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(run_proc_sb), prefs.run_processes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(run_load_sb), (gfloat)prefs.run_load / 10.0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit1_check), prefs.edit_first_error);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editw_check), !prefs.edit_warnings);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fail_check), prefs.ignore_failures);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(emm_check), prefs.enable_make_makefile);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soo_check), prefs.scroll_on_output);
    gtk_entry_set_text(GTK_ENTRY(makefile_entry), (prefs.makefile == 0 ? "" : prefs.makefile));
    ui_combo_set_current(start_action_combo, prefs.start_action);
    ui_combo_set_current(finish_action_combo, prefs.finish_action);

    gtk_clist_freeze(GTK_CLIST(var_clist));
    gtk_clist_clear(GTK_CLIST(var_clist));
    for (list = prefs.variables ; list != 0 ; list = list->next)
    {
	Variable *var = (Variable *)list->data;
	char *text[VC_MAX];

	text[VC_NAME] = var->name;
	text[VC_VALUE] = var->value;
	text[VC_TYPE] = var_type_to_str(var->type);
	gtk_clist_append(GTK_CLIST(var_clist), text);
    }
    gtk_clist_thaw(GTK_CLIST(var_clist));
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), "");
    ui_combo_set_current(var_type_combo, 0);

    gtk_entry_set_text(GTK_ENTRY(prog_make_entry), prefs.prog_make);
    gtk_entry_set_text(GTK_ENTRY(prog_targets_entry), prefs.prog_list_targets);
    gtk_entry_set_text(GTK_ENTRY(prog_version_entry), prefs.prog_list_version);
    gtk_entry_set_text(GTK_ENTRY(prog_edit_entry), prefs.prog_edit_source);
    gtk_entry_set_text(GTK_ENTRY(prog_make_makefile_entry), prefs.prog_make_makefile);
    gtk_entry_set_text(GTK_ENTRY(prog_finish_entry), prefs.prog_finish);
    gtk_widget_set_sensitive(prog_finish_entry, (prefs.finish_action == FINISH_COMMAND));

    /* TODO: need to set these 2 combos to the current values */
    ui_combo_set_current(paper_name_combo, 0);
    ui_combo_set_current(paper_orientation_combo, 0);
    prefs_set_units_entry(paper_width_entry, prefs.paper_width);
    prefs_set_units_entry(paper_height_entry, prefs.paper_height);
    prefs_set_units_entry(margin_left_entry, prefs.margin_left);
    prefs_set_units_entry(margin_right_entry, prefs.margin_right);
    prefs_set_units_entry(margin_top_entry, prefs.margin_top);
    prefs_set_units_entry(margin_bottom_entry, prefs.margin_bottom);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
prefs_create_shell(GtkWidget *toplevel)
{
    GtkWidget *box;
    GtkWidget *page;

    prefs_shell = ui_create_apply_dialog(toplevel, _("Maketool: Preferences"),
    	prefs_apply_cb, (gpointer)0);
    ui_autonull_pointer(&prefs_shell);
    
    creating = TRUE;
    
    box = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(prefs_shell)->vbox), box);
    gtk_container_border_width(GTK_CONTAINER(box), SPACING);
    gtk_widget_show(box);
    
    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(box), notebook);
    gtk_widget_show(notebook);

    page = prefs_create_general_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("General")));
    gtk_widget_show(page);

    
    page = prefs_create_variables_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("Variables")));
    gtk_widget_show(page);

    page = prefs_create_programs_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("Programs")));
    gtk_widget_show(page);

    page = prefs_create_colors_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("Colors")));
    gtk_widget_show(page);

    page = prefs_create_page_setup_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("Page Setup")));
    page_setup_index = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page);
    gtk_widget_show(page);

#if 0    
    page = prefs_create_styles_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new(_("Styles")));
    gtk_widget_show(page);
#endif

    creating = FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
edit_preferences_cb(GtkWidget *w, gpointer data)
{
    if (prefs_shell == 0)
	prefs_create_shell(toplevel);
	    
    prefs_show();
    gtk_widget_show(prefs_shell);
}

void
print_page_setup_cb(GtkWidget *w, gpointer data)
{
    if (prefs_shell == 0)
	prefs_create_shell(toplevel);
	    
    prefs_show();
    gtk_notebook_set_page(GTK_NOTEBOOK(notebook), page_setup_index);
    gtk_widget_show(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
