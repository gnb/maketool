#include <sys/types.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

#define SPACING 5

static GtkWidget	*prefs_shell = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
prefs_apply_cb(GtkWidget *w, gpointer data)
{
    fprintf(stderr, "prefs_apply_cb()\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
changed_cb(GtkWidget *w, gpointer data)
{
    uiDialogChanged(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static GtkWidget *
prefs_create_general_page(GtkWidget *toplevel)
{
    GtkWidget *table, *table2;
    GtkWidget *para_radio[3];
    GtkWidget *frame;
    GtkObject *para_jflag_adj;
    GtkObject *para_lflag_adj;
    GtkWidget *sb;
    GtkWidget *check;
    GtkWidget *label;
    GtkWidget *combo;
    GList *list;
    int row = 0;
    
    table = gtk_table_new(1, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_widget_show(table);
    
    /*
     * `Parallel' frame
     */
    frame = gtk_frame_new("Make runs commands");
    gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, row, row+1);
    gtk_widget_show(frame);
    row++;
    
    table2 = gtk_table_new(3, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table2);
    gtk_widget_show(table2);
    
    para_radio[0] = gtk_radio_button_new_with_label_from_widget(0,
    			"in series.");
    gtk_signal_connect(GTK_OBJECT(para_radio[0]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), para_radio[0], 0, 2, 0, 1);
    gtk_widget_show(para_radio[0]);
    

    para_radio[1] = gtk_radio_button_new_with_label_from_widget(
    			GTK_RADIO_BUTTON(para_radio[0]),
    			"in parallel. Maximum number of processes is ");
    gtk_signal_connect(GTK_OBJECT(para_radio[1]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), para_radio[1], 0, 1, 1, 2);
    gtk_widget_show(para_radio[1]);

    para_jflag_adj = gtk_adjustment_new(
    		(gfloat)2, 	/* value */
                (gfloat)2,	/* lower */
                (gfloat)10,	/* upper */
                (gfloat)1,	/* step_increment */
                (gfloat)5,	/* page_increment */
                (gfloat)0);	/* page_size */
    sb = gtk_spin_button_new(GTK_ADJUSTMENT(para_jflag_adj), 0.5, 0);		
    gtk_signal_connect(GTK_OBJECT(sb), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), sb, 1, 2, 1, 2);
    gtk_widget_show(sb);

    
    
    para_radio[2] = gtk_radio_button_new_with_label_from_widget(
    			GTK_RADIO_BUTTON(para_radio[0]),
    			"in parallel. Maximum machine load is ");
    gtk_signal_connect(GTK_OBJECT(para_radio[2]), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), para_radio[2], 0, 1, 2, 3);
    gtk_widget_show(para_radio[2]);
    
    para_lflag_adj = gtk_adjustment_new(
    		(gfloat)2.0, 	/* value */
                (gfloat)0.1,	/* lower */
                (gfloat)5.0,	/* upper */
                (gfloat)0.1,	/* step_increment */
                (gfloat)0.5,	/* page_increment */
                (gfloat)0);	/* page_size */
    sb = gtk_spin_button_new(GTK_ADJUSTMENT(para_lflag_adj), 0.5, 1);		
    gtk_signal_connect(GTK_OBJECT(sb), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table2), sb, 1, 2, 2, 3);
    gtk_widget_show(sb);

    /*
     * Edit first detected error.
     */
    check = gtk_check_button_new_with_label("Auto edit first error");
    gtk_signal_connect(GTK_OBJECT(check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), check, 0, 2, row, row+1);
    gtk_widget_show(check);
    row++;
    
    /*
     * Auto edit warnings
     */
    check = gtk_check_button_new_with_label("Auto edit warnings");
    gtk_table_attach_defaults(GTK_TABLE(table), check, 0, 2, row, row+1);
    gtk_signal_connect(GTK_OBJECT(check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_widget_show(check);
    row++;
    
    /*
     * -k flag
     */
    /* TODO: better name */
    check = gtk_check_button_new_with_label("Continue despite failures");
    gtk_signal_connect(GTK_OBJECT(check), "toggled", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), check, 0, 2, row, row+1);
    gtk_widget_show(check);
    row++;
    
    /*
     * Action on starting build.
     */
    label = gtk_label_new("On starting each build: ");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    		/* TODO: use option menu? */
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, "Clear log");
    list = g_list_append(list, "Collapse all log items");
    list = g_list_append(list, "Do nothing");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_widget_show(combo);
    row++;


    return table;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *variables_titles[] = { "Name", "Value", "Type" };


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
    
    vbox = gtk_vbox_new(SPACING, FALSE);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);
    gtk_widget_show(vbox);
    
    hbox = gtk_hbox_new(0, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    clist = gtk_clist_new_with_titles(3, variables_titles);
    gtk_box_pack_start(GTK_BOX(hbox), clist, TRUE, TRUE, 0);
    gtk_widget_show(clist);
    
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
    label = gtk_label_new("Name:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    /*
     * Value
     */
    label = gtk_label_new("Value:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    /*
     * Type
     */
    label = gtk_label_new("Type:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    

    /*
     * Set & Unset buttons
     */
    hbox = gtk_hbox_new(SPACING, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);
    
    button = gtk_button_new_with_label("Set");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("Unset");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);


    /* TODO: do this for real in a separate fn */
    text[0] = "VARIABLE_ONE";
    text[1] = "The value for ONE";
    text[2] = "make";
    gtk_clist_append(GTK_CLIST(clist), text);
    text[0] = "VARIABLE_TWO";
    text[1] = "The value for TWO";
    text[2] = "make";
    gtk_clist_append(GTK_CLIST(clist), text);
    text[0] = "VARIABLE_THREE";
    text[1] = "The value for THREE";
    text[2] = "environ";
    gtk_clist_append(GTK_CLIST(clist), text);
    text[0] = "VARIABLE_FOUR";
    text[1] = "The value for FOUR";
    text[2] = "make";
    gtk_clist_append(GTK_CLIST(clist), text);
    text[0] = "VARIABLE_FIVE";
    text[1] = "The value for FIVE";
    text[2] = "environ";
    gtk_clist_append(GTK_CLIST(clist), text);
    text[0] = "VARIABLE_SIX";
    text[1] = "The value for SIX";
    text[2] = "make";
    gtk_clist_append(GTK_CLIST(clist), text);

    return vbox;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static GtkWidget *
prefs_create_systems_page(GtkWidget *toplevel)
{
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *entry;
    int row = 0;
    
    table = gtk_table_new(1, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_widget_show(table);
    
    
    label = gtk_label_new("Editor:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "...systems...");
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    return table;
}
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char programs_key[] = "\
Key\n\
%f              source filename\n\
%l              source line number\n\
%m              -f makefile if specified\n\
%p              parallel flags (-j or -k)\n\
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
    int row = 0;
    
    table = gtk_table_new(4, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_widget_show(table);
    
    /*
     * Make program
     */
    label = gtk_label_new("Make:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "make %m %p %v %t");
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    /*
     * Make target lister
     */
    label = gtk_label_new("List make targets:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "extract_targets %m");
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;

    /*
     * Make version lister
     */
    label = gtk_label_new("List make version:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "make --version");
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;

    /*
     * Editor
     */
    label = gtk_label_new("Edit source files:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "nc -noask %{l:+-line %l} %f");
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
    	GTK_SIGNAL_FUNC(changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, row, row+1);
    gtk_widget_show(entry);
    
    row++;
    
    /*
     * Key
     */
    label = gtk_label_new(programs_key);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, row, row+1);
    gtk_widget_show(label);
    row++;
    
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
    
    
    label = gtk_label_new("Editor:");
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

    prefs_shell = uiCreateApplyDialog(toplevel, "Maketool: Preferences",
    	prefs_apply_cb, (gpointer)0);

    box = gtk_vbox_new(0, FALSE);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(prefs_shell)->vbox), box);
    gtk_container_border_width(GTK_CONTAINER(box), SPACING);
    gtk_widget_show(box);
    
    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(box), notebook);
    gtk_widget_show(notebook);

    
    page = prefs_create_general_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new("General"));
    gtk_widget_show(page);

    
    page = prefs_create_variables_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new("Variables"));
    gtk_widget_show(page);

#if 0    
    page = prefs_create_systems_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new("Systems"));
    gtk_widget_show(page);
#endif
    
    page = prefs_create_programs_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new("Programs"));
    gtk_widget_show(page);

#if 0    
    page = prefs_create_styles_page(toplevel);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page,
    				gtk_label_new("Styles"));
    gtk_widget_show(page);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
preferences_show(GtkWidget *toplevel)
{
    if (prefs_shell == 0)
	prefs_create_shell(toplevel);
	    
    gtk_widget_show(prefs_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
