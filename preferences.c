#include "ui.h"
#include "maketool.h"
#include "util.h"

CVSID("$Id: preferences.c,v 1.8 1999-05-25 08:02:48 gnb Exp $");

static GtkWidget	*prefs_shell = 0;
static GtkWidget	*run_proc_sb;
static GtkWidget	*run_load_sb;
static GtkWidget	*edit1_check;
static GtkWidget	*editw_check;
static GtkWidget	*fail_check;
static GtkWidget	*run_radio[3];
static GtkWidget	*prog_make_entry;
static GtkWidget	*prog_targets_entry;
static GtkWidget	*prog_version_entry;
static GtkWidget	*prog_edit_entry;
static GtkWidget	*start_action_combo;
static GtkWidget	*makefile_entry;
static GtkWidget	*var_clist;
static GtkWidget	*var_name_entry;
static GtkWidget	*var_value_entry;
static GtkWidget	*var_type_entry;
static GtkWidget	*var_set_btn;
static GtkWidget	*var_unset_btn;
static gboolean		creating = TRUE, setting = FALSE;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
prefs_add_variable(const char *name, const char *value, int type)
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
	const char *p;
	
	if (var->type != VAR_MAKE)
	    continue;
	
    	if (out.length > 0)
	    estring_append_char(&out, ' ');
	estring_append_string(&out, var->name);
	estring_append_char(&out, '=');
	for (p = var->value ; *p ; p++)
	{
	    if (strchr(shell_metas, *p))
		estring_append_char(&out, '\\');
	    estring_append_char(&out, *p);
	}
    }
    
    if (prefs.var_make_flags != 0)
    	g_free(prefs.var_make_flags);
    prefs.var_make_flags = out.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
preferences_init(void)
{
    prefs.run_how = RUN_SERIES;
    prefs.run_processes = 2;
    prefs.run_load = 2.0;
        
    prefs.edit_first_error = FALSE;
    prefs.edit_warnings = TRUE;
    prefs.ignore_failures = FALSE;
    
    prefs.start_action = START_COLLAPSE;
    prefs.makefile = 0;
    
    prefs.variables = 0;
    prefs_add_variable("VARIABLE_ONE", "The value for 'ONE'", VAR_MAKE);
    prefs_add_variable("VARIABLE_TWO", "'The' value for TWO", VAR_MAKE);
    prefs_add_variable("VARIABLE_THREE", "'The' value for THREE", VAR_ENVIRON);
    prefs_add_variable("VARIABLE_FOUR", "The value for FOUR", VAR_MAKE);
    prefs_add_variable("VARIABLE_FIVE", "The value for 'FIVE'", VAR_ENVIRON);
    prefs_add_variable("VARIABLE_SIX", "The value for SIX", VAR_MAKE);
    prefs_set_var_environment();
    prefs_set_var_make_flags();
    
    prefs.prog_make = g_strdup("make %m %k %p %v %t");
    prefs.prog_list_targets = g_strdup("extract_targets %m %v");
    prefs.prog_list_version = g_strdup("make --version");
    prefs.prog_edit_source = g_strdup("nc -noask %{l:+-line %l} %f");
}


void
preferences_load(void)
{
}

void
preferences_save(void)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
prefs_apply_cb(GtkWidget *w, gpointer data)
{
    char *mf;
    char *text[3];
    int row, nrows;
    
    fprintf(stderr, "prefs_apply_cb()\n");
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run_radio[RUN_PARALLEL_PROC])))
    	prefs.run_how = RUN_PARALLEL_PROC;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run_radio[RUN_PARALLEL_LOAD])))
    	prefs.run_how = RUN_PARALLEL_LOAD;
    else
    	prefs.run_how = RUN_SERIES;

        
    prefs.run_processes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(run_proc_sb));
    prefs.run_load = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(run_load_sb));
    
    prefs.edit_first_error = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit1_check));
    prefs.edit_warnings = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editw_check));
    prefs.ignore_failures = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fail_check));

    free(prefs.prog_make);
    prefs.prog_make = strdup(gtk_entry_get_text(GTK_ENTRY(prog_make_entry)));
        
    free(prefs.prog_list_targets);
    prefs.prog_list_targets = strdup(gtk_entry_get_text(GTK_ENTRY(prog_targets_entry)));
    
    free(prefs.prog_list_version);
    prefs.prog_list_version = strdup(gtk_entry_get_text(GTK_ENTRY(prog_version_entry)));
    
    free(prefs.prog_edit_source);
    prefs.prog_edit_source = strdup(gtk_entry_get_text(GTK_ENTRY(prog_edit_entry)));
    
    prefs.start_action = uiComboGetCurrent(start_action_combo);
    
    /* TODO: strip `mf' of whitespace JIC */
    mf = gtk_entry_get_text(GTK_ENTRY(makefile_entry));
    if (prefs.makefile != 0)
	free(prefs.makefile);
    prefs.makefile = (mf == 0 || *mf == '\0' ? 0 : strdup(mf));
    
    prefs_clear_variables();
    nrows = GTK_CLIST(var_clist)->rows;        
    for (row = 0 ; row < nrows ; row++)
    {
	gtk_clist_get_text(GTK_CLIST(var_clist), row, 0, &text[0]);
	gtk_clist_get_text(GTK_CLIST(var_clist), row, 1, &text[1]);
	gtk_clist_get_text(GTK_CLIST(var_clist), row, 2, &text[2]);
	prefs_add_variable(text[0], text[1],
		(!strcmp(text[2], "make") ? VAR_MAKE : VAR_ENVIRON));
    }
    prefs_set_var_environment();
    prefs_set_var_make_flags();
      
    preferences_save();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
changed_cb(GtkWidget *w, gpointer data)
{
    if (!creating)
	uiDialogChanged(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
set_makefile(const char *mf)
{
    gtk_entry_set_text(GTK_ENTRY(makefile_entry), mf);
    uiDialogChanged(prefs_shell);
}

static void
browse_makefile_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    char *mf = gtk_entry_get_text(GTK_ENTRY(makefile_entry));
    
    if (filesel == 0)
    	filesel = uiCreateFileSel(_("Choose Makefile"), set_makefile, mf);
    else
    	gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), mf);

    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static GtkWidget *
prefs_create_general_page(GtkWidget *toplevel)
{
    GtkWidget *table, *table2, *hbox;
    GtkWidget *frame;
    GtkObject *adj;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *button;
    GList *list;
    int row = 0;
    
    table = gtk_table_new(6, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_widget_show(table);

    
    /*
     * `Parallel' frame
     */
    frame = gtk_frame_new(_("Make runs commands"));
    gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, row, row+1);
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
    gtk_widget_show(run_radio[RUN_SERIES]);
    

    run_radio[RUN_PARALLEL_PROC] = gtk_radio_button_new_with_label_from_widget(
    			GTK_RADIO_BUTTON(run_radio[RUN_SERIES]),
    			_("in parallel. Maximum number of processes is "));
    gtk_signal_connect(GTK_OBJECT(run_radio[RUN_PARALLEL_PROC]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), run_radio[RUN_PARALLEL_PROC], 0, 1, 1, 2);
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
    gtk_table_attach_defaults(GTK_TABLE(table), edit1_check, 0, 2, row, row+1);
    gtk_widget_show(edit1_check);
    row++;
    
    /*
     * Auto edit warnings
     */
    editw_check = gtk_check_button_new_with_label(_("Edit Next ignores warnings"));
    gtk_signal_connect(GTK_OBJECT(editw_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), editw_check, 0, 2, row, row+1);
    gtk_widget_show(editw_check);
    row++;
    
    /*
     * -k flag
     */
    fail_check = gtk_check_button_new_with_label(_("Continue despite failures"));
    gtk_signal_connect(GTK_OBJECT(fail_check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), fail_check, 0, 2, row, row+1);
    gtk_widget_show(fail_check);
    row++;

    /*
     * Action on starting build.
     */
    label = gtk_label_new(_("On starting each build: "));
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    		/* TODO: use option menu? */
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, _("Do nothing"));
    list = g_list_append(list, _("Clear log"));
    list = g_list_append(list, _("Collapse all log items"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_widget_show(combo);
    start_action_combo = combo;
    row++;


    /*
     * Choose Makefile.
     */
    
    label = gtk_label_new(_("Makefile:"));
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(hbox), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, row, row+1);
    gtk_widget_show(hbox);

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
    gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);	
    gtk_widget_show(combo);
    makefile_entry = GTK_COMBO(combo)->entry;

    button = gtk_button_new_with_label(_("Browse..."));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(browse_makefile_cb), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, SPACING);	
    gtk_widget_show(button);

    row++;

    /* 
     * Set current values. This is isolated at the bottom
     * of this function in case it needs to be separated later.
     */
    gtk_toggle_button_set_active(
    	GTK_TOGGLE_BUTTON(run_radio[prefs.run_how]), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(run_proc_sb), prefs.run_processes);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(run_load_sb), prefs.run_load);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit1_check), prefs.edit_first_error);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editw_check), !prefs.edit_warnings);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fail_check), prefs.ignore_failures);
    gtk_entry_set_text(GTK_ENTRY(makefile_entry), (prefs.makefile == 0 ? "" : prefs.makefile));
    uiComboSetCurrent(start_action_combo, prefs.start_action);

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
    char *text[3];

    gtk_clist_get_text(GTK_CLIST(var_clist), row, 0, &text[0]);
    gtk_clist_get_text(GTK_CLIST(var_clist), row, 1, &text[1]);
    gtk_clist_get_text(GTK_CLIST(var_clist), row, 2, &text[2]);
    
    setting = TRUE;
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), text[0]);
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), text[1]);
    gtk_entry_set_text(GTK_ENTRY(var_type_entry), text[2]);
    setting = FALSE;
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, TRUE);
}


static void
var_changed_cb(GtkWidget *w, gpointer data)
{
    if (!creating && !setting)
    {
	gtk_widget_set_sensitive(var_set_btn, TRUE);
	gtk_widget_set_sensitive(var_unset_btn, TRUE);
    }
}

static void
var_set_cb(GtkWidget *w, gpointer data)
{
    int row;
    char *text[3];
    char *rtext[3];
    int nrows = GTK_CLIST(var_clist)->rows;
    
    text[0] = gtk_entry_get_text(GTK_ENTRY(var_name_entry));
    text[1] = gtk_entry_get_text(GTK_ENTRY(var_value_entry));
    text[2] = gtk_entry_get_text(GTK_ENTRY(var_type_entry));
        
    for (row = 0 ; row < nrows ; row++)
    {
	gtk_clist_get_text(GTK_CLIST(var_clist), row, 0, &rtext[0]);
	if (!strcmp(rtext[0], text[0]))
	{
	    gtk_clist_get_text(GTK_CLIST(var_clist), row, 1, &rtext[1]);
	    gtk_clist_get_text(GTK_CLIST(var_clist), row, 2, &rtext[2]);
	    if (!strcmp(text[1], rtext[1]) && !strcmp(text[2], rtext[2]))
	    	return;	/* same variable, no change to value or type: ignore */
	    gtk_clist_set_text(GTK_CLIST(var_clist), row, 1, text[1]);
	    gtk_clist_set_text(GTK_CLIST(var_clist), row, 2, text[2]);
	    break;
	}
    }
    /* got here, so must not be any row with same variable name */
    
    gtk_clist_append(GTK_CLIST(var_clist), text);
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, TRUE);
    uiDialogChanged(prefs_shell);
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
	gtk_clist_get_text(GTK_CLIST(var_clist), row, 0, &rname);
	if (!strcmp(rname, name))
	{
	    gtk_clist_remove(GTK_CLIST(var_clist), row);
	    changed = TRUE;
	    break;
	}
    }

    setting = TRUE;
    gtk_entry_set_text(GTK_ENTRY(var_name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(var_value_entry), "");
    gtk_entry_set_text(GTK_ENTRY(var_type_entry), "");
    setting = FALSE;
    gtk_widget_set_sensitive(var_set_btn, FALSE);
    gtk_widget_set_sensitive(var_unset_btn, FALSE);
    if (changed)
	uiDialogChanged(prefs_shell);
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
    GtkWidget *sb;
    char *text[3];
    int row = 0;
    
    vbox = gtk_vbox_new(FALSE, SPACING);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);
    gtk_widget_show(vbox);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    text[0] = _("Name");
    text[1] = _("Value");
    text[2] = _("Type");
    clist = gtk_clist_new_with_titles(3, text);
    gtk_clist_set_sort_column(GTK_CLIST(clist), 0);
    gtk_clist_set_auto_sort(GTK_CLIST(clist), TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 2, TRUE);
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
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(var_changed_cb), 0);
    gtk_widget_show(entry);
    var_type_entry = entry;
    
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


    /* TODO: do this for real in a separate fn */
    {
        GList *list;
	
	for (list = prefs.variables ; list != 0 ; list = list->next)
	{
	    Variable *var = (Variable *)list->data;
	    int r;
	    text[0] = var->name;
	    text[1] = var->value;
	    text[2] = (var->type == VAR_MAKE ? "make" : "environ");
	    r = gtk_clist_append(GTK_CLIST(clist), text);
	}
    }

    return vbox;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char programs_key[] = "\
Key\n\
%f              source filename\n\
%l              source line number\n\
%m              -f makefile if specified\n\
%p              parallel flags (-j or -l)\n\
%k              continue flag (-k) if specified\n\
%v              make variable overrides\n\
%t              make target\n\
%{x:+y}         y if %x is not empty\n\
";



static GtkWidget *
prefs_create_programs_page(GtkWidget *toplevel)
{
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *combo;
    GList *list;
    int row = 0;
    
    table = gtk_table_new(4, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    gtk_widget_show(table);
    
    /*
     * Make program
     */
    label = gtk_label_new(_("Make:"));
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
     * Key
     */
    label = gtk_label_new(programs_key);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, row, row+1);
    gtk_widget_show(label);
    row++;
    
    /* 
     * Set current values. This is isolated at the bottom
     * of this function in case it needs to be separated later.
     */
    
    gtk_entry_set_text(GTK_ENTRY(prog_make_entry), prefs.prog_make);
    gtk_entry_set_text(GTK_ENTRY(prog_targets_entry), prefs.prog_list_targets);
    gtk_entry_set_text(GTK_ENTRY(prog_version_entry), prefs.prog_list_version);
    gtk_entry_set_text(GTK_ENTRY(prog_edit_entry), prefs.prog_edit_source);


    return table;
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

static void
prefs_create_shell(GtkWidget *toplevel)
{
    GtkWidget *notebook;
    GtkWidget *box;
    GtkWidget *page;

    prefs_shell = uiCreateApplyDialog(toplevel, _("Maketool: Preferences"),
    	prefs_apply_cb, (gpointer)0);

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
view_preferences_cb(GtkWidget *w, gpointer data)
{
    if (prefs_shell == 0)
	prefs_create_shell(toplevel);
	    
    gtk_widget_show(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/