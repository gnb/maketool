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
#include "log.h"
#include "ui.h"
#include "util.h"

CVSID("$Id: print.c,v 1.10 2001-07-25 12:02:44 gnb Exp $");

static GtkWidget	*print_shell = 0;
typedef enum { D_PRINTER, D_FILE, D_NUM_DESTS } DEST;
static GtkWidget    	*dest_radio[D_NUM_DESTS];
static GtkWidget    	*file_entry;
static GtkWidget    	*printer_entry;

/*
 * List of strings which are the simple names of printers
 * TODO: handle separate name/description
 */
static GList	    	*lpr_printers = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean _lpr_initialised = FALSE;

static void
_lpr_add_printer(const char *name)
{
#if DEBUG > 5
    fprintf(stderr, "Adding printer \"%s\"\n", name);
#endif
    lpr_printers = g_list_append(lpr_printers, g_strdup(name));
}

/*
 * UNIX print system specific functions.
 */
 
#if HAVE_ETC_PRINTCAP

/* TODO: have a default-printer variable & assign it here */

void
lpr_init(void)
{
    FILE *fp;
    gboolean in_entry = FALSE;
    char buf[1024];
    
    if (_lpr_initialised)
    	return;
    _lpr_initialised = TRUE;
    
    if ((fp = fopen("/etc/printcap", "r")) == 0)
    {
    	perror("/etc/printcap");
	return;
    }
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	char *x;
	if ((x = strchr(buf, '\n')) != 0)
	    *x = 0;
	if ((x = strchr(buf, '\r')) != 0)
	    *x = 0;
	    
	if (buf[0] == '#')
	    continue;	/* comment line */
	    
	if (in_entry && buf[strlen(buf)-1] == '\\')
	    continue;	    /* ignore everything except the first line */
	if (isspace(buf[0]))
	    continue;	    /* ignore empty lines */

    	/* first :-seperated field is list of names |-seperated */
	x = strchr(buf, ':');
	if (x != 0)
	    *x = '\0';
	x = strchr(buf, '|');
	if (x != 0)
	    *x = '\0';
	
    	_lpr_add_printer(buf);
    }
    
    fclose(fp);
}

char *
_lpr_job_command(const char *printer)
{
    return g_strdup_printf("lpr -P\"%s\"", printer);
}

#elif HAVE_LPSTAT

void
lpr_init(void)
{
    FILE *fp;
    char *cmd;
    char buf[1024];
    
    if (_lpr_initialised)
    	return;
    _lpr_initialised = TRUE;
    
    /* TODO: check that lpstat -c (IIRC) and output are correct */
    cmd = g_strdup_printf("%s -c", LPSTAT_COMMAND);
    fp = popen(cmd, "r");
    g_free(cmd);
    
    if (fp == 0)
    {
    	perror(LPSTAT_COMMAND);
	return;
    }

    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	char *x;
	if ((x = strchr(buf, '\n')) != 0)
	    *x = 0;
	if ((x = strchr(buf, '\r')) != 0)
	    *x = 0;
    	_lpr_add_printer(buf);
    }
    
    pclose(fp);
}

char *
_lpr_job_command(const char *printer)
{
    return g_strdup_printf("lp -d \"%s\"", printer);
}

#else

void
lpr_init(void)
{
    fprintf(stderr, "No printer support compiled into this executable\n");
}

char *
_lpr_job_command(const char *printer)
{
    return g_strdup("echo No printer support compiled into this executable");
}

#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

FILE *
lpr_job_begin(const char *printer)
{
    FILE *fp;
    char *cmd;
    
    cmd = _lpr_job_command(printer);
    message("Printing using: %s", cmd);
    fp = popen(cmd, "w");
    g_free(cmd);
    
    return fp;
}

void
lpr_job_end(FILE *fp)
{
    pclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static DEST
print_get_dest(void)
{
    int i;
    
    for (i=0 ; i<D_NUM_DESTS ; i++)
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dest_radio[i])))
    	    return (DEST)i;
    return (DEST)0; 	/* default value */

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
print_ok_cb(GtkWidget *w, gpointer data)
{
    FILE *fp;
    char *printer, *file;
    DEST dest = print_get_dest();
    
    switch (dest)
    {
    case D_PRINTER:
    	printer = gtk_entry_get_text(GTK_ENTRY(printer_entry));
	if ((fp = lpr_job_begin(printer)) == 0)
	{
    	    /* TODO: lpr_ API should allow us an error message */
    	    message(_("Can't create job on printer `%s'"), printer);
	    return;
	}
	log_print(fp);
	lpr_job_end(fp);
    	break;
    
    case D_FILE:
    	/* TODO: check filename */
	file = gtk_entry_get_text(GTK_ENTRY(file_entry));
	message("Printing to file %s", file);
	fp = fopen(file, "w");
	log_print(fp);
	fclose(fp);
    	break;
	
    default:
    	fprintf(stderr, "Unknown destination %d\n", (int)dest);
	break;
    }

    gtk_widget_hide(print_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
print_cancel_cb(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(print_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
print_file_func(const char *filename)
{
    gtk_entry_set_text(GTK_ENTRY(file_entry), filename);
}

static void
print_file_browse_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget *filesel = 0;
    
    if (filesel == 0)
    {
    	filesel = ui_create_file_sel(
	    	toplevel,
	    	_("Maketool: Choose Output File"),
		print_file_func,
		gtk_entry_get_text(GTK_ENTRY(file_entry)));
    }

    /* TODO: set current filename from file_entry *every* time */
    /* TODO: modal popup */
    
    gtk_widget_show(filesel);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
print_dest_changed_cb(GtkWidget *w, gpointer data)
{
    DEST dest = print_get_dest();
    int i;
    
    for (i=0 ; i<D_NUM_DESTS ; i++)
    	ui_group_set_sensitive(GR_PRINT_PRINTER+i, (i == dest));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
create_print_shell(void)
{
    GtkWidget *table;
    GtkWidget *combo;
    GtkWidget *button;
    GList *list;
    int row = 0;

    /* get list of printers from system */
    lpr_init();
    
    print_shell = ui_create_dialog(toplevel, _("Maketool: Print"));

    gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(print_shell)->vbox), SPACING);

    table = gtk_table_new(3, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), SPACING);
    gtk_table_set_row_spacings(GTK_TABLE(table), SPACING);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(print_shell)->vbox), table);
    gtk_widget_show(table);
    
    dest_radio[D_PRINTER] = gtk_radio_button_new_with_label_from_widget(
    	    	0, _("Print to printer"));
    gtk_signal_connect(GTK_OBJECT(dest_radio[D_PRINTER]), "toggled", 
    	GTK_SIGNAL_FUNC(print_dest_changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), dest_radio[D_PRINTER], 0, 1, row, row+1);
    gtk_widget_show(dest_radio[D_PRINTER]);
    
    
    combo = gtk_combo_new();
    list = 0;
    if (lpr_printers != 0)
	list = g_list_copy(lpr_printers);
    else
	list = g_list_append(list, _("no printers on this system"));
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, row, row+1);
    gtk_widget_show(combo);
    printer_entry = GTK_COMBO(combo)->entry;
    ui_group_add(GR_PRINT_PRINTER, combo);

    row++;    
    
    dest_radio[D_FILE] = gtk_radio_button_new_with_label_from_widget(
    	    	GTK_RADIO_BUTTON(dest_radio[D_PRINTER]), _("Print to file"));
    gtk_signal_connect(GTK_OBJECT(dest_radio[D_PRINTER]), "toggled", 
    	GTK_SIGNAL_FUNC(print_dest_changed_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), dest_radio[D_FILE], 0, 1, row, row+1);
    gtk_widget_show(dest_radio[D_FILE]);
    
    combo = gtk_combo_new();
    list = 0;
    list = g_list_append(list, "maketool.ps");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
    gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 2, row, row+1);
    gtk_widget_show(combo);
    file_entry = GTK_COMBO(combo)->entry;
    ui_group_add(GR_PRINT_FILE, combo);

    button = gtk_button_new_with_label(_("Browse..."));
    gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	GTK_SIGNAL_FUNC(print_file_browse_cb), 0);
    gtk_table_attach(GTK_TABLE(table), button, 2, 3, row, row+1,
		       (GtkAttachOptions)0, (GtkAttachOptions)0,
		       SPACING, 0);
    gtk_widget_show(button);
    ui_group_add(GR_PRINT_FILE, button);

    row++;
    
    ui_dialog_create_button(print_shell, _("OK"),
    	print_ok_cb, (gpointer)0);
    ui_dialog_create_button(print_shell, _("Page Setup..."),
    	print_page_setup_cb, (gpointer)0);
    ui_dialog_create_button(print_shell, _("Cancel"),
    	print_cancel_cb, (gpointer)0);

    if (lpr_printers == 0)
    {	
    	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dest_radio[D_FILE]), TRUE);
    	gtk_widget_set_sensitive(dest_radio[D_PRINTER], FALSE);
    }
    print_dest_changed_cb((GtkWidget*)0, (gpointer)0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
file_print_cb(GtkWidget *w, gpointer data)
{
    if (print_shell == 0)
    	create_print_shell();
    gtk_widget_show(print_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
