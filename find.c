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
#include "log.h"
#include "ui.h"
#include "util.h"
#include <regex.h>	/* POSIX regular expression fns */
#include <gdk/gdkkeysyms.h>

CVSID("$Id: find.c,v 1.2 2000-01-08 14:33:13 gnb Exp $");

static GtkWidget	*find_shell = 0;
typedef enum { FD_FORWARDS, FD_BACKWARDS, FD_MAX_DIRECTIONS } FindDirections;
typedef enum
{
    FT_LITERAL,     	/* case-INsensitive literal */
    FT_CASE_LITERAL,	/* case-sensitive literal */
    FT_REGEXP,	    	/* regular expression */
    FT_MAX_TYPES
} FindTypes;
static GtkWidget    	*dirn_radio[FD_MAX_DIRECTIONS];
static GtkWidget    	*type_radio[FT_MAX_TYPES];
static GtkWidget    	*string_entry;
static FindDirections	direction = FD_FORWARDS;

typedef struct 
{
    FindTypes type;
    char *string;
    regex_t regexp;
} FindState;

static GList *find_state_stack = 0;
static GList *find_state_current = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG

static void
find_state_print(FILE *fp, const FindState *s)
{
    const char *type_str = 0;
    const char *dirn_str = 0;
    
    switch (s->type)
    {
    case FT_LITERAL:     	/* case-INsensitive literal */
    	type_str = "insensitive-literal";
	break;
    case FT_CASE_LITERAL:	/* case-sensitive literal */
    	type_str = "sensitive-literal";
	break;
    case FT_REGEXP:	    	/* regular expression */
    	type_str = "regexp";
	break;
    default:
    	type_str = "unknown";
	break;
    }
    
    fprintf(fp, "{ type=%s string=\"%s\" }",
    	type_str, dirn_str, s->string);
}

#endif

static int
find_state_compare(const FindState *s1, const FindState *s2)
{
    if (s1->type != s2->type)
    	return (int)s1->type - (int)s2->type;
    return strcmp(s1->string, s2->string);
}

static gboolean
find_state_get(FindState *state)
{
    int i;
    
    state->type = -1;
    state->string = 0;

    /* get current type */
    for (i=0 ; i<FT_MAX_TYPES ; i++)
    {
    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(type_radio[i])))
	{
	    state->type = i;
	    break;
	}
    }
    if (state->type == -1)
    	return FALSE;
    
    /* TODO: separately reinit these */
    state->string = gtk_entry_get_text(GTK_ENTRY(string_entry));
    if (state->type == FT_REGEXP)
    {
    	guint err;
        if ((err = regcomp(&state->regexp, state->string, REG_EXTENDED|REG_NOSUB)) != 0)
	{
            char errbuf[1024];
    	    regerror(err, &state->regexp, errbuf, sizeof(errbuf));
	    /* TODO: decent error reporting mechanism */
	    fprintf(stderr, _("%s: error in regular expression \"%s\": %s\n"),
		    argv0, state->string, errbuf);
	    regfree(&state->regexp);
	    return FALSE;
	}
    }
    return TRUE;
}

static void
find_state_set(const FindState *state)
{
    int i;
    
    /* set current type */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type_radio[state->type]), TRUE);
    
    gtk_entry_set_text(GTK_ENTRY(string_entry), state->string);
}

static void
find_state_push(FindState *s)
{
#if DEBUG
    fprintf(stderr, "pushing find state: ");
    find_state_print(stderr, s);
    fprintf(stderr, "\n");
#endif
    find_state_stack = g_list_prepend(find_state_stack, s);
    find_state_current = find_state_stack;
}

static FindState *
find_state_find(const FindState *proto)
{
    GList *link = g_list_find_custom(find_state_stack, (gpointer)proto,
    	(GCompareFunc)find_state_compare);
    return (link == 0 ? 0 : (FindState *)link->data);
}


/*
 * Notice that `prev' wrt to the user is backwards to
 * `prev' wrt the stack linkage.
 */
static void
find_state_prev(void)
{
    if (find_state_current != 0 &&
        find_state_current->next != 0)
    {
    	find_state_current = find_state_current->next;
	find_state_set((FindState *)find_state_current->data);
    }
}

static void
find_state_next(void)
{
    if (find_state_current != 0 &&
        find_state_current->prev != 0)
    {
    	find_state_current = find_state_current->prev;
	find_state_set((FindState *)find_state_current->data);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* returns FALSE to stop iteration */
static gboolean
find_apply_func(LogRec *lr, gpointer user_data)
{
    FindState *state = (FindState *)user_data;
    gboolean found = FALSE;
    
    switch (state->type)
    {
    case FT_LITERAL:	    /* case-INsensitive literal */
    	/* TODO: implement this properly */
    	found = (strstr(log_get_text(lr), state->string) != 0);
	break;
    case FT_CASE_LITERAL:   /* case-sensitive literal */
    	found = (strstr(log_get_text(lr), state->string) != 0);
	break;
    case FT_REGEXP: 	    /* regular expression */
        found = !regexec(&state->regexp, log_get_text(lr), 0, 0, 0);
	break;
    }
    
    if (found)
    	log_set_selected(lr);
    return !found;
}


static void
do_find(void)
{
    /* TODO: implement finding backwards */
    FindState *state, proto;
    
    memset(&proto, 0, sizeof(proto));
    if (!find_state_get(&proto))
    	return;
	
    state = find_state_find(&proto);
    if (state == 0)
    {
    	/* TODO: enforce a max length of the stack */
#if DEBUG
    	fprintf(stderr, "do_find: creating new state\n");
#endif
    	state = g_new(FindState, 1);
	memcpy(state, &proto, sizeof(*state));
	state->string = g_strdup(state->string);
	find_state_push(state);
    }
    else
    {
#if DEBUG
    	fprintf(stderr, "do_find: reusing old state\n");
#endif
    	find_state_stack = g_list_remove(find_state_stack, state);
	find_state_push(state);
    }
    
    /* get current direction */
    direction = (gtk_toggle_button_get_active(
    	GTK_TOGGLE_BUTTON(dirn_radio[FD_FORWARDS])) ?
    	FD_FORWARDS : FD_BACKWARDS);

    /* do the actual search */    
    log_apply_after(
    	find_apply_func,
	(direction == FD_FORWARDS),
	log_selected(),
	(gpointer)state);
    
    /* The Find Again menu item is now available */
    grey_menu_items();
}

static void
do_find_again(void)
{
    FindState *state;

    /* TODO: implement finding backwards */
    assert(find_state_stack != 0);
    state = (FindState *)find_state_stack->data;

    /* use previous direction */
    
    /* do the actual search */    
    log_apply_after(
    	find_apply_func,
	(direction == FD_FORWARDS),
	log_selected(),
	(gpointer)state);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
find_find_cb(GtkWidget *w, gpointer data)
{
    do_find();
}

static void
find_cancel_cb(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(find_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
find_keypress_cb(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
    static const char regexp_metachars[] = "[]*+()^$";
    
    switch (event->key.keyval)
    {
    case GDK_Page_Up:
    	find_state_prev();
    	break;
    case GDK_Page_Down:
    	find_state_next();
    	break;
    default:
    	/*
	 * If the user types (rather than pastes) in characters
	 * which indicate he's probably going to use a regexp,
	 * make it a regexp for him. He can always set it back.
	 */
    	if (event->key.length == 1 &&
	    strchr(regexp_metachars, event->key.string[0]) != 0)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type_radio[FT_REGEXP]), TRUE);
    	break;
    }
    return TRUE;    /* keep processing */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
create_find_shell(void)
{
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *radio;
    GtkWidget *entry;
    int row = 0;

    find_shell = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(find_shell), _("Maketool: Find"));

    gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(find_shell)->vbox), SPACING);

    hbox = gtk_hbox_new(FALSE, 0);
    /*gtk_container_border_width(GTK_CONTAINER(hbox), SPACING);*/
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(find_shell)->vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    label = gtk_label_new(_("Search for:"));    /* TODO: better label */
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show(entry);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
            GTK_SIGNAL_FUNC(find_find_cb), 0);    

    gtk_signal_connect(GTK_OBJECT(entry), "key-press-event",
            GTK_SIGNAL_FUNC(find_keypress_cb), 0);    


    string_entry = entry;


    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(find_shell)->vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    radio = gtk_radio_button_new_with_label_from_widget(0, _("Literal"));
    gtk_box_pack_start(GTK_BOX(hbox), radio, FALSE, TRUE, 0);
    gtk_widget_show(radio);
    type_radio[FT_LITERAL] = radio;
    
    radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio), _("Case Sensitive Literal"));
    gtk_box_pack_start(GTK_BOX(hbox), radio, FALSE, TRUE, 0);
    gtk_widget_show(radio);
    type_radio[FT_CASE_LITERAL] = radio;
    
    radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio), _("Regular Expression"));
    gtk_box_pack_start(GTK_BOX(hbox), radio, FALSE, TRUE, 0);
    gtk_widget_show(radio);
    type_radio[FT_REGEXP] = radio;


    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(find_shell)->vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    radio = gtk_radio_button_new_with_label_from_widget(0, _("Search Forwards"));
    gtk_box_pack_start(GTK_BOX(hbox), radio, FALSE, TRUE, 0);
    gtk_widget_show(radio);
    dirn_radio[FD_FORWARDS] = radio;
    
    radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio), _("Search Backwards"));
    gtk_box_pack_start(GTK_BOX(hbox), radio, FALSE, TRUE, 0);
    gtk_widget_show(radio);
    dirn_radio[FD_BACKWARDS] = radio;
    
    ui_dialog_create_button(find_shell, _("Find"),
    	find_find_cb, (gpointer)0);
    ui_dialog_create_button(find_shell, _("Cancel"),
    	find_cancel_cb, (gpointer)0);
}

/* TODO: grey out Find button in Find dialog when no string entered
 * or log is empty */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
edit_find_cb(GtkWidget *w, gpointer data)
{
    if (find_shell == 0)
    	create_find_shell();
    /* TODO: populate entry widget with current selection */
    gtk_widget_show(find_shell);
}

void
edit_find_again_cb(GtkWidget *w, gpointer data)
{
    do_find_again();
}

gboolean
find_can_find_again(void)
{
    return (find_state_stack != 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
