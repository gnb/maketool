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
#include "ui.h"
#include "util.h"
#include "task.h"

Task *logged_task(char *command);

#define strassign(x, s) \
    do { \
    	if ((x) != 0) \
	    g_free(x); \
	(x) = g_strdup(s); \
    } while(0)

#define strfree(x) \
    do { \
    	if ((x) != 0) \
	{ \
	    g_free(x); \
	    (x) = 0; \
	} \
    } while(0)

typedef enum
{
    CAT_NONE,
    CAT_CONFIG,
    CAT_DIRECTORY,
    CAT_HOST,
    CAT_FEATURE,
    
    NUM_CATEGORIES
}
category_t;

#define W_CHECK 0
#define W_TEXT 1

typedef enum
{
    ARG_NONE,
    ARG_FIXED,
    ARG_OPTIONAL
}
argtype_t;

#define OPT_ADVANCED	(1<<0)	    /* appears only in Advanced mode */
#define OPT_PRESENT	(1<<1)	    /* option has been set by user or config.status */
#define OPT_NOMETA	(1<<2)	    /* no metacharacters allowed in value */
#define OPT_NOSPACE	(1<<3)	    /* no whitespace allowed in value */
#define OPT_SILENT	(1<<4)	    /* no option_t created */

typedef struct
{
    char *name;
    category_t category;
    argtype_t argtype;
    char *metavar;
    char *help;
    char *defvalue;
    char *value;
    unsigned int flags;
    GtkWidget *hbox;
    GtkWidget *widgets[3];
}
option_t;

typedef struct
{
    const char *name;
    int value;
} benum_t;

static category_t category = CAT_NONE;
static estring optline = ESTRING_STATIC_INIT;
static const char *category_names[NUM_CATEGORIES] =
{
    "-none-",
    N_("Configuration"),
    N_("Directory and file names"),
    N_("Host type"),
    N_("Features and packages")
};
static GList *all_options;
static const benum_t initial_opt_flags[] = 
{
{"cache-file",	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"no-create",	    	    OPT_ADVANCED},
{"quiet,",  	    	    OPT_SILENT},
{"quiet",   	    	    OPT_ADVANCED},
{"prefix",     	    	    OPT_NOSPACE|OPT_NOMETA},
{"exec-prefix",     	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"bindir",  	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"sbindir", 	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"libexecdir",	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"datadir", 	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"sysconfdir",	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"sharedstatedir",  	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"localstatedir",   	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"libdir",  	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"includedir",	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"oldincludedir",   	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"infodir", 	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"mandir",  	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"srcdir",  	    	    OPT_ADVANCED|OPT_NOSPACE|OPT_NOMETA},
{"program-prefix",  	    OPT_ADVANCED|OPT_NOSPACE},
{"program-suffix",  	    OPT_ADVANCED|OPT_NOSPACE},
{"program-transform-name",  OPT_ADVANCED},
{"host",    	    	    OPT_ADVANCED|OPT_NOSPACE},
{"build",   	    	    OPT_ADVANCED|OPT_NOSPACE},
{"target",  	    	    OPT_ADVANCED|OPT_NOSPACE},
{"x-includes",	    	    OPT_ADVANCED},
{"x-libraries",     	    OPT_ADVANCED},
{"enable-maintainer-mode",  OPT_ADVANCED},
{"help",    	    	    OPT_SILENT},
{"version", 	    	    OPT_SILENT},
{"disable-FEATURE", 	    OPT_SILENT},
{"enable-FEATURE",  	    OPT_SILENT},
{"with-PACKAGE",    	    OPT_SILENT},
{"without-PACKAGE", 	    OPT_SILENT},
{0,0}
};

static const char *advanced_btn_names[] =
{
    N_("Advanced..."),
    N_("Basic..."),
};


#if DEBUG
static const benum_t opt_flag_desc[] = 
{
{"advanced",	OPT_ADVANCED},
{"present", 	OPT_PRESENT},
{"nometa",  	OPT_NOMETA},
{"nospace", 	OPT_NOSPACE},
{"silent",  	OPT_SILENT},
{0, 0}
};
#endif


static GtkWidget *autoconf_shell;
static GtkWidget *advanced_btn;
static GtkWidget *notebook;
static GtkWidget *category_pages[NUM_CATEGORIES];
static GtkWidget *preview_page;
static GtkWidget *preview_text;
static GtkTooltips *tooltips;
static gboolean advanced = FALSE;



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
benum_lookup_str(const benum_t *be, const char *str)
{
    for ( ; be->name != 0 ; be++)
    {
    	if (!strcmp(be->name, str))
	    return be->value;
    }
    return -1;
}

#if DEBUG
static char *
benum_describe_bits(const benum_t *be, int val)
{
    estring e;
    
    if (val == 0)
    	return g_strdup("0");

    estring_init(&e);
    
    /* find any matching benum entries and use them as description */
    for ( ; be->name != 0 ; be++)
    {
    	if ((val & be->value) == be->value)
	{
	    if (e.length > 0)
		estring_append_char(&e, ',');
	    estring_append_string(&e, be->name);
	    val &= ~be->value;
	}
    }

    /* describe any unnamed bits as numeric literals */
    if (val != 0)
    {
    	int i;
	
    	for (i=0 ; i<32 ; i++)
	{
	    if ((unsigned)val & (1<<i))
	    {
		if (e.length > 0)
		    estring_append_char(&e, ',');
		estring_append_printf(&e, "1<<%d", i);
	    }
	}
    }
    
    return e.data;
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Returns a new string containing the fully shell-escaped
 * configure command with all selected options.
 */
static char *
build_configure_command(void)
{
    GList *list;
    estring cmd;
        

    estring_init(&cmd);
    
    estring_append_string(&cmd, "./configure");
    
    for (list = all_options ; list != 0 ; list = list->next)
    {
    	option_t *opt = (option_t *)list->data;
	char *val;
	
	if (!(opt->flags & OPT_PRESENT))
	    continue;

	estring_append_printf(&cmd, " --%s", opt->name);

	switch (opt->argtype)
	{
	case ARG_NONE:
	    val = 0;
	    break;
	case ARG_FIXED:
	    val = opt->value;
	    if (val == 0)
	    	val = "";
	    break;
	case ARG_OPTIONAL:
	    val = opt->value;
	    if (val != 0 && *val == '\0')
	    	val = 0;
	    break;
	}
	
	if (val != 0)
	{
	    val = strescape(val);
	    estring_append_printf(&cmd, "=%s", val);
	    g_free(val);
	}
    }
    
    return cmd.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
add_option(
    category_t cat,
    const char *name,
    argtype_t argtype,
    const char *metavar,
    const char *help,
    const char *def)
{
    option_t *opt;
    char *defval;
    int flags;
    
    flags = benum_lookup_str(initial_opt_flags, name);
    if (flags == -1)
    	flags = 0;  /* don't know about this one, should be OK */
    
    /* Handle some well-known meta-options and real options which are
     * not useful in the GUI, by pretending they don't exist.
     */
    if (flags & OPT_SILENT)
    	return;
	

    /* hack to make PREFIX and EPREFIX metavars in autoconf defaults work */
    if (def == 0)
    	defval = 0;
    else if (!strprefix(def, "PREFIX/"))
    	defval = g_strconcat("${prefix}", def+6, 0);
    else if (!strprefix(def, "EPREFIX/"))
    	defval = g_strconcat("${exec_prefix}", def+7, 0);
    else if (!strcmp(name, "exec-prefix"))
    	defval = g_strdup("${prefix}");
    else
    	defval = g_strdup(def);
	
#if DEBUG
    {
    	char *s;
	
	s = benum_describe_bits(opt_flag_desc, flags);
	fprintf(stderr,
    	    "OPTION{\n  category=\"%s\"\n  name=\"%s\"\n  argtype=%d\n  flags=%s\n  metavar=\"%s\"\n  help=\"%s\"\n  default=\"%s\"\n}\n\n",
    	    category_names[category],
	    name,
	    argtype,
    	    s,
	    metavar,
	    help,
	    defval);
	g_free(s);
    }
#endif


    opt = g_new(option_t, 1);
    memset(opt, 0, sizeof(*opt));
    
    opt->name = g_strdup(name);
    opt->category = cat;
    opt->argtype = argtype;
    opt->metavar = g_strdup(metavar);
    opt->help = g_strdup(help);
    opt->defvalue = defval;
    opt->value = g_strdup(defval);
    opt->flags = flags;
    
    all_options = g_list_append(all_options, opt);
    
    /* Hack to present help better.  Works for LANG=en. */
    if (islower(opt->help[0]))
    	opt->help[0] = toupper(opt->help[0]);
}

    
static void
delete_option(option_t *opt)
{
    if (opt->hbox != 0)
    {
    	gtk_widget_hide(opt->hbox);
	gtk_widget_destroy(opt->hbox);
    }

    strfree(opt->name);
    strfree(opt->metavar);
    strfree(opt->help);
    strfree(opt->defvalue);
    strfree(opt->value);
    g_free(opt);
}


static option_t *
find_option(const char *name)
{
    GList *list;
    
    for (list = all_options ; list != 0 ; list = list->next)
    {
    	option_t *opt = (option_t *)list->data;
	
	if (!strcmp(opt->name, name))
	    return opt;
    }
    
    return 0;
}

static GList *
select_options(category_t cat)
{
    GList *res = 0;
    GList *list;
    
    for (list = all_options ; list != 0 ; list = list->next)
    {
    	option_t *opt = (option_t *)list->data;
	
	if (opt->category == cat)
	    res = g_list_append(res, opt);
    }
    
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_option_line(void)
{
    const char *x;
    estring opt, metavar, help, def;
    argtype_t argtype = ARG_NONE;

    estring_init(&opt);
    estring_init(&metavar);
    estring_init(&help);
    estring_init(&def);

    /* parse option name */
    for (x = optline.data ; *x && !isspace(*x) && *x != '=' && *x != '[' ; x++)
	estring_append_char(&opt, *x);
	
    if (*x == '=')
    {
    	/* parse meta argument */
	argtype = ARG_FIXED;
	
	for (x++ ; *x && !isspace(*x) ; x++)
	    estring_append_char(&metavar, *x);
    }
    else if (x[0] == '[' && x[1] == '=')
    {
    	/* parse meta argument */
	argtype = ARG_OPTIONAL;
	
	for (x+=2 ; *x && *x != ']' ; x++)
	    estring_append_char(&metavar, *x);
    }
    
    /* skip some spaces */
    for (x++ ; *x && isspace(*x) ; x++)
	;
	
    /* parse help */
    for ( ; *x && *x != '[' ; x++)
	estring_append_char(&help, *x);
    
    /* parse default */
    if (*x == '[')
    {
	for (x++ ; *x && *x != ']' ; x++)
	    estring_append_char(&def, *x);
    }
    

    add_option(category, opt.data, argtype, metavar.data, help.data, def.data);

    estring_free(&opt);
    estring_free(&metavar);
    estring_free(&help);
    estring_free(&def);
}

static void
handle_line(const char *line)
{
#if DEBUG
    fprintf(stderr, "==> \"%s\"\n", line);
#endif
    
    /* Skip known fluff */
    if (!strprefix(line, "Usage:") ||
        !strprefix(line, "Options:"))
    	return;
	
    
    /* handle option lines and their continuations */
    if (!strprefix(line, "  --"))
    {
    	if (optline.length > 0)
    	    parse_option_line();
	estring_truncate(&optline);
	estring_append_string(&optline, line+4);
    }
    else if (isspace(line[0]) &&
    	     isspace(line[1]) &&
	     isspace(line[2]))
    {
    	/* continuation line */
	
	/* first skip leading whitespace -- crunch to single space */
	while (*line && isspace(*line))
	    line++;
	estring_append_char(&optline, ' ');
	
	/* add the rest of the list */
	estring_append_string(&optline, line);
    }
    else
    {
    	/* Categories etc */
    	if (optline.length > 0)
    	    parse_option_line();
	estring_truncate(&optline);

	/* Parse known categories */
	if (!strprefix(line, "Configuration:"))
	{
    	    category = CAT_CONFIG;
	}
	else if (!strprefix(line, "Directory and file names:"))
	{
    	    category = CAT_DIRECTORY;
	}
	else if (!strprefix(line, "Host type:"))
	{
    	    category = CAT_HOST;
	}
	else if (!strprefix(line, "Features and packages:"))
	{
    	    category = CAT_FEATURE;
	}
	else if (!strprefix(line, "--enable and --with options recognized:"))
	{
    	    category = CAT_FEATURE;
	}
    }
}


static void
handle_input(int len, const char *buf)
{
    static estring leftover = ESTRING_STATIC_INIT;
    
    if (len == 0)
    {
    	/* end of input */
	/*
	 * Handle case where last line of child process'
	 * output has no terminating '\n'. Beware - 
	 * this last line may contain an error or warning,
	 * which affects log_num_{errors,warnings}().
	 */
	if (leftover.length > 0)
	    handle_line(leftover.data);
	/*
	 * Free'ing and reinitialising seems a bit extreme,
	 * but it allows the program to give back to malloc()
	 * a large line buffer allocated for a single unusual
	 * very long line.
	 */
	estring_free(&leftover);
	estring_init(&leftover);
	return;
    }
    
    while (len > 0 && *buf)
    {
	char *p = strchr(buf, '\n');
	if (p == 0)
	{
    	    /* only a part of a line left - append to leftover */
	    estring_append_string(&leftover, buf);
	    return;
	}
    	/* got an end-of-line - isolate the line & feed it to handle_line() */
	*p = '\0';
	estring_append_string(&leftover, buf);

    	handle_line(leftover.length > 0 ? leftover.data : "");
	
	estring_truncate(&leftover);
	len -= (p - buf);
	buf = ++p;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
configure_help_input(Task *task, int len, const char *buf)
{
    handle_input(len, buf);
}

static void
configure_help_reap(Task *task)
{
    
    handle_input(0, 0);     /* deal with possibly unterminated last line */
    
    /* parse last option */
    if (optline.length > 0)
    	parse_option_line();
    estring_free(&optline);


#if DEBUG
    fprintf(stderr, "DONE\n");
#endif
}


static TaskOps configure_help_ops =
{
0,  	    	    	    	/* start */
configure_help_input,	    	/* input */
configure_help_reap, 	    	/* reap */
0   	    		    	/* destroy */
};



static void
read_configure_options(void)
{
    Task *task;
    
    
    /* Delete all previously read options */
    while (all_options != 0)
    {
    	delete_option((option_t *)all_options->data);
	all_options = g_list_remove_link(all_options, all_options);
    }
    
    task = task_create(
    	(Task *)g_new(Task, 1),
	g_strdup("./configure --help"),
	/*env*/0,
	&configure_help_ops);
    task_enqueue(task);     /* TODO: wrong */
    task_start();
    
    while (task_is_running())
    	g_main_iteration(/*block*/TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
parse_config_status_line(const char *buf)
{
    estring name, value;
    int quote;
    option_t *opt;
    
    estring_init(&name);
    estring_init(&value);
    
    /* Skip leading characters */
    buf += 2;
    
    /* Skip configure script filename */
    for ( ; buf && !isspace(*buf) ; buf++)
    	;
	
    /* 
     * Parse arguments.
     */
    for (;;)
    {
	/* skip whitespace */
	for ( ; buf && isspace(*buf) ; buf++)
    	    ;
	if (*buf == '\0')
	    break;
	    
	/* handle quoted arguments */
	quote = (*buf == '\'' ? '\'' : 0);
	if (quote)
	    buf++;
	
	estring_truncate(&name);
	estring_truncate(&value);
	
	if (buf[0] != '-' || buf[1] != '-')
	{
	    fprintf(stderr, "ERROR: option \"%s\" does not begin with --\n", buf);
	    break;
	}
	buf += 2;
	
	/* parse option name */
	for ( ; *buf && (quote ? *buf != quote : !isspace(*buf)) && *buf != '=' ; buf++)
	    estring_append_char(&name, *buf);
	    
	if (*buf == '=')
	{
	    buf++;
	    /* parse option value */
	    for ( ; *buf && (quote ? *buf != quote : !isspace(*buf)) ; buf++)
		estring_append_char(&value, *buf);
	}
	
	if (quote && *buf == quote)
	    buf++;
	    
#if DEBUG
    	fprintf(stderr, "NAME=\"%s\" VALUE=\"%s\"\n", name.data, value.data);
#endif
	
	if ((opt = find_option(name.data)) == 0)
	{
	    fprintf(stderr, "ERROR: unknown option \"%s\"\n", name.data);
	    continue;
	}
	
	if (opt->value != 0)
	    g_free(opt->value);
	opt->value = g_strdup(value.data);
	opt->flags |= OPT_PRESENT;
    }
    
    estring_free(&name);
    estring_free(&value);
}


static void
parse_config_status(void)
{
    FILE *fp;
    char buf[1024];
    
    if ((fp = fopen("config.status", "r")) == 0)
    {
    	if (errno != ENOENT)
	    perror("config.status");
	return;
    }

    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	if (!strprefix(buf, "# ./configure") ||
	    !strprefix(buf, "# configure"))
	{
	    parse_config_status_line(buf);
	    break;
	}
    }    
    
    fclose(fp);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
apply_advanced(void)
{
    GList *list;
    int cat;
    
    /* set visibility of each option */
    for (list = all_options ; list != 0 ; list = list->next)
    {
    	option_t *opt = (option_t *)list->data;
	
	ui_widget_set_visible(opt->hbox, (!(opt->flags & OPT_ADVANCED) || advanced));
	if (opt->widgets[W_TEXT] != 0 && GTK_WIDGET_VISIBLE(opt->widgets[W_TEXT]))
	    gtk_widget_set_sensitive(opt->widgets[W_TEXT], (opt->flags & OPT_PRESENT));
    }
    
    /* set visiblity of containing notebook pages */
    for (cat=1 ; cat<NUM_CATEGORIES ; cat++)
    {
	GtkWidget *sw = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), cat-1);
	GtkWidget *page = category_pages[cat];

    	ui_widget_set_visible(sw,
	    (ui_container_num_visible_children(GTK_CONTAINER(page)) > 0));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
update_preview(void)
{
    char *cmd = build_configure_command();

    gtk_text_freeze(GTK_TEXT(preview_text));
    gtk_text_set_point(GTK_TEXT(preview_text), 0);
    gtk_text_forward_delete(GTK_TEXT(preview_text),
    	    	    gtk_text_get_length(GTK_TEXT(preview_text)));
    gtk_text_insert(GTK_TEXT(preview_text), /*font*/0, /*fore*/0,
    	    	    	/*back*/0, cmd, strlen(cmd));
    gtk_text_thaw(GTK_TEXT(preview_text));
            
    g_free(cmd);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
autoconf_cancel_cb(GtkWidget *w, gpointer data)
{
/*    DPRINTF("autoconf_cancel_cb\n"); */
    
    gtk_widget_hide(autoconf_shell);
}

static void
autoconf_advanced_cb(GtkWidget *w, gpointer data)
{
    
/*    DPRINTF("autoconf_advanced_cb\n"); */
    
    advanced = !advanced;
    gtk_label_set_text(GTK_LABEL(GTK_BIN(advanced_btn)->child),
    	    	    	  _(advanced_btn_names[advanced]));
    apply_advanced();
}


static void
autoconf_ok_cb(GtkWidget *w, gpointer data)
{
    
/*    DPRINTF("autoconf_ok_cb\n"); */

    /* Hide the dialog window, and wait until it goes away */    
    gtk_widget_hide(autoconf_shell);
    while (g_main_pending())
    	g_main_iteration(/*may_block*/FALSE);

    task_enqueue(logged_task(build_configure_command()));
    task_start();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
on_check_toggled(GtkWidget *w, gpointer data)
{
    option_t *opt = (option_t *)data;
        
/*    DPRINTF("on_check_toggled\n"); */

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
	opt->flags |= OPT_PRESENT;
    else
	opt->flags &= ~OPT_PRESENT;

    if (opt->widgets[W_TEXT] != 0)
    {
    	gtk_widget_set_sensitive(opt->widgets[W_TEXT], opt->flags & OPT_PRESENT);
	if (opt->flags & OPT_PRESENT)
	{
	    gtk_widget_grab_focus(opt->widgets[W_TEXT]);
	    gtk_editable_select_region(GTK_EDITABLE(opt->widgets[W_TEXT]), 0, -1);
	}
    }
    
    update_preview();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
on_text_changed(GtkWidget *w, gpointer data)
{
    option_t *opt = (option_t *)data;
    gboolean valid = TRUE;
    const char *newval = gtk_entry_get_text(GTK_ENTRY(w));
    const char *p;
        
/*    DPRINTF("on_text_changed\n"); */

    /* perform basic field validation */
    if (opt->flags & OPT_NOMETA)
    {
    	for (p = newval ; *p ; p++)
	{
	    if (ismetachar(*p))
	    {
		valid = FALSE;
		break;
	    }
	}
    }
    
    if (valid && (opt->flags & OPT_NOSPACE))
    {
    	for (p = newval ; *p ; p++)
	{
	    if (isspace(*p))
	    {
		valid = FALSE;
		break;
	    }
	}
    }
    
    if (!valid)
    {
	gtk_entry_set_text(GTK_ENTRY(w), opt->value);
	gdk_beep();
    	return;
    }
    
    /* valid change, so save the new value */
    strassign(opt->value, newval);
    
    update_preview();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
create_option_widgets(void)
{
    int cat;
    GList *options;
    int row;
    GtkWidget *label;
    
    for (cat = 1 ; cat <NUM_CATEGORIES ; cat++)
    {
	options = select_options(cat);
	if (options == 0)
    	    continue;



	row = 0;
	while (options != 0)
	{
	    GtkWidget *check;
	    GtkWidget *text;
    	    option_t *opt = (option_t *)options->data;

	    opt->hbox = gtk_hbox_new(/*homogenous*/FALSE, /*spacing*/SPACING);
	    gtk_box_pack_start(GTK_BOX(category_pages[cat]), opt->hbox,
    	    	    		/*expand*/FALSE, /*fill*/TRUE, /*padding*/0);
#if DEBUG
    	    gtk_widget_set_name(opt->hbox, opt->name);
#endif
	    gtk_widget_show(opt->hbox);

	    check = gtk_check_button_new_with_label(_(opt->help));
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), opt->flags & OPT_PRESENT);
	    gtk_signal_connect(GTK_OBJECT(check), "toggled", 
    		GTK_SIGNAL_FUNC(on_check_toggled), opt);
	    gtk_tooltips_set_tip(tooltips, check, opt->name, 0);
	    gtk_label_set_line_wrap(GTK_LABEL(GTK_BIN(check)->child), TRUE);
	    gtk_box_pack_start(GTK_BOX(opt->hbox), check,
    	    	    		/*expand*/FALSE, /*fill*/TRUE, /*padding*/0);
	    gtk_widget_show(check);
	    opt->widgets[W_CHECK] = check;

    	    if (opt->argtype != ARG_NONE)
	    {
		text = gtk_entry_new();
		if (opt->value != 0)
		    gtk_entry_set_text(GTK_ENTRY(text), opt->value);
		gtk_signal_connect(GTK_OBJECT(text), "changed", 
    		    GTK_SIGNAL_FUNC(on_text_changed), opt);
		gtk_widget_set_sensitive(text, opt->flags & OPT_PRESENT);
		gtk_tooltips_set_tip(tooltips, text, opt->name, 0);
		gtk_box_pack_start(GTK_BOX(opt->hbox), text,
    	    	    		    /*expand*/TRUE, /*fill*/TRUE, /*padding*/0);
		gtk_widget_show(text);
		opt->widgets[W_TEXT] = text;
    	    }

	    options = g_list_remove_link(options, options);
	    row++;
	}

    }
    apply_advanced();
}

static const char preview_str[] = N_("\
The command line of the configure script will appear \
like this, according to the options you have chosen.\
");

static void
create_autoconf_shell(void)
{
    GtkWidget *button;
    GtkWidget *sw;
    GtkWidget *page;
    GtkWidget *label;
    int cat;
    
    autoconf_shell = ui_create_dialog(toplevel,  _("Maketool: Configure Options"));
    ui_set_help_name(autoconf_shell, "configure-window");
    
/*    gtk_container_border_width(GTK_CONTAINER(autoconf_shell), SPACING); */
    
    /* TODO: centralize with ui.c */ 
    tooltips = gtk_tooltips_new();
 
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(autoconf_shell)->vbox), notebook,
    	    	    	/*expand*/TRUE, /*fill*/TRUE, /*padding*/0);
			
    /* create a page for each category */
    for (cat = 1 ; cat <NUM_CATEGORIES ; cat++)
    {
	sw = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    	    	    	    	    /*hscrollbar_policy*/GTK_POLICY_NEVER,
				    /*vscrollbar_policy*/GTK_POLICY_AUTOMATIC);
	gtk_container_border_width(GTK_CONTAINER(sw), SPACING);

	page = gtk_vbox_new(/*homogenous*/FALSE, /*spacing*/SPACING);
	gtk_container_border_width(GTK_CONTAINER(page), SPACING);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), page);

	gtk_widget_show(page);
	gtk_widget_show(sw);
	category_pages[cat] = page;

	label = gtk_label_new(_(category_names[cat]));
	gtk_widget_show(label);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);
    }

    
    /*
     * Create the preview page.
     */
    preview_page = gtk_vbox_new(/*homogenous*/FALSE, /*spacing*/SPACING);
    gtk_container_border_width(GTK_CONTAINER(preview_page), SPACING);
    
    label = gtk_label_new(_(preview_str));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(preview_page), label,
    	    	    	/*expand*/FALSE, /*fill*/TRUE, /*padding*/0);
    gtk_widget_show(label);
    
    preview_text = gtk_text_new(0, 0);
    gtk_text_set_editable(GTK_TEXT(preview_text), FALSE);
    gtk_box_pack_start(GTK_BOX(preview_page), preview_text,
    	    	    	/*expand*/TRUE, /*fill*/TRUE, /*padding*/0);
    gtk_widget_show(preview_text);

    gtk_widget_show(preview_page);

    label = gtk_label_new(_("Preview"));
    gtk_widget_show(label);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), preview_page, label);
    
    
    gtk_widget_show(notebook);
    

    /* 
     * Create the action area.
     */
    button = ui_dialog_create_button(autoconf_shell,
    	    	    	    _("OK"),
    	    	    	    autoconf_ok_cb, 0);
    gtk_window_set_default(GTK_WINDOW(autoconf_shell), button);
    advanced_btn = ui_dialog_create_button(autoconf_shell,
    	    	    	    _(advanced_btn_names[advanced]),
			    autoconf_advanced_cb, 0);
    ui_dialog_create_button(autoconf_shell,
    	    	    	    _("Cancel"),
			    autoconf_cancel_cb, 0);
}



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
build_configure_cb(GtkWidget *w, gpointer data)
{
    read_configure_options();
    parse_config_status();
    if (autoconf_shell == 0)
    	create_autoconf_shell();
    create_option_widgets();
    update_preview();
    gtk_widget_show(autoconf_shell);
}

void
build_autoconf_cb(GtkWidget *w, gpointer data)
{
    task_enqueue(logged_task(g_strdup("autoconf")));
    task_start();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
check_for_configure_in(void)
{
    FILE *fp;
    
    fp = fopen("configure.in", "r");
    if (fp == 0)
    	return FALSE;
	
    fclose(fp);
    return TRUE;
}

gboolean
check_for_configure(void)
{
    FILE *fp;
    gboolean gotmagic;
    int lines;
    char buf[1024];
    static const char magic[] = "# Generated automatically using autoconf ";
#define MAXLINES 20
    
    fp = fopen("configure", "r");
    if (fp == 0)
    	return FALSE;
	
    /*
     * Some bizarre packages include a "configure" script which
     * is *not* autoconf-generated, so for our purposes (e.g.
     * parsing the output of "configure --help") it doesn't exist.
     */
    /* First, check its some kind of script */
    if (fgetc(fp) != '#' && fgetc(fp) != '!')
    {
    	fclose(fp);
    	return FALSE;
    }
    fgets(buf, sizeof(buf), fp);    /* skip to end of line */
    
    /* Second, check for the magic string */
    gotmagic = FALSE;
    for (lines = 0 ;
    	 lines < MAXLINES && fgets(buf, sizeof(buf), fp) != 0 ;
	 lines++)
    {
    	if (!strprefix(buf, magic))
	{
	    gotmagic = TRUE;
	    break;
	}
    }
    if (!gotmagic)
    {
    	fclose(fp);
    	return FALSE;
    }
    
    /* well it seems ok */
	
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
