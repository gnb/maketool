#include <sys/types.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include "spawn.h"


const char	*argv0;
char		**targets;
int		numTargets;
int		currentTarget;
GtkWidget	*toplevel;
GtkWidget	*menubar, *toolbar, *messagebox;
GtkWidget	*logwin, *messageent;
GList		*widgets_notempty, *widgets_notrunning, *widgets_running;
pid_t		currentPid = -1;
gint		currentInput;
int		currentFd;

#define ANIM_MAX 8
GdkPixmap	*anim_pixmaps[ANIM_MAX+1];
GdkBitmap	*anim_masks[ANIM_MAX+1];
GtkWidget	*anim;

#define SPACING 4

#define PASTE3(x,y,z) x##y##z

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
message(const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    
    gtk_entry_set_text(GTK_ENTRY(messageent), buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
widget_list_set_sensitive(GList *list, gboolean b)
{
    while (list != 0)
    {
    	gtk_widget_set_sensitive(GTK_WIDGET(list->data), b);
    	list = list->next;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
grey_menu_items(void)
{
    gboolean running = (currentPid > 0);
    gboolean empty = (gtk_text_get_length(GTK_TEXT(logwin)) > 0);

    widget_list_set_sensitive(widgets_notrunning, !running);
    widget_list_set_sensitive(widgets_running, running);
    widget_list_set_sensitive(widgets_notempty, empty);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
reapMake(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    char *target = (char *)user_data;
    
    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
	message("Finished making %s", target);
	if (currentPid == pid)
	    currentPid = -1;
	gtk_input_remove(currentInput);
	close(currentFd);
	grey_menu_items();
    }
}


static void
handleInput(gpointer data, gint source, GdkInputCondition condition)
{
    if (source == currentFd && condition == GDK_INPUT_READ)
    {
    	int nremain = 0;
	char buf[1024];
	
	if (ioctl(currentFd, FIONREAD, &nremain) < 0)
	{
	    perror("ioctl(FIONREAD)");
	    return;
	}
	while (nremain > 0)
	{
	    gboolean do_grey;
	    int n = read(currentFd, buf, MIN(sizeof(buf), nremain));
	    nremain -= n;
	    /* TODO: split into lines and classify */
	    do_grey = (gtk_text_get_length(GTK_TEXT(logwin)) == 0);
	    gtk_text_insert(GTK_TEXT(logwin), 0, 0, 0, buf, n);
	    grey_menu_items();
	}
    }
}


#define READ 0
#define WRITE 1
#define STDOUT 1
void
buildStart(const char *target)
{
    pid_t pid;
    int pipefds[2];
    char buf[2048];
    
    sprintf(buf, "make %s", target);
    
    
    if (pipe(pipefds) < 0)
    {
    	perror("pipe");
    	return;
    }
    
    if ((pid = fork()) < 0)
    {
    	/* error */
    	perror("fork");
    }
    else if (pid == 0)
    {
    	/* child */
	close(pipefds[READ]);
	dup2(pipefds[WRITE], STDOUT);
	close(pipefds[WRITE]);
	
	execlp("/bin/sh", "/bin/sh", "-c", buf, 0);
	perror("execlp");
    }
    else
    {
    	/* parent */
	message("Making %s", target);
	spawn_add_reap_func(pid, reapMake, (gpointer)target);
	currentPid = pid;
	currentFd = pipefds[READ];
	close(pipefds[WRITE]);
	currentInput = gdk_input_add(currentFd,
			    GDK_INPUT_READ, handleInput, (gpointer)0);
	grey_menu_items();
    }
}
#undef READ
#undef WRITE
#undef STDOUT

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
file_exit_cb(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_cb(GtkWidget *w, gpointer data)
{
    buildStart((char *)data);    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_widget_cb(GtkWidget *w, gpointer data)
{
    GtkWidget **wp = (GtkWidget **)data;
    
    if (GTK_CHECK_MENU_ITEM(w)->active)
    	gtk_widget_show(*wp);
    else
    	gtk_widget_hide(*wp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
clear_log_cb(GtkWidget *w, gpointer data)
{
    GtkText *lw = GTK_TEXT(logwin);
    
    gtk_text_set_point(lw, 0);
    gtk_text_forward_delete(lw, gtk_text_get_length(lw));
    grey_menu_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0

static int anim_current = 0;

static void
anim_advance(void)
{
    if (anim_current++ == ANIM_MAX)
    	anim_current = 1;
    gtk_pixmap_set(GTK_PIXMAP(anim),
    	anim_pixmaps[anim_current], anim_masks[anim_current]);
}


static gint timer = -1;

static gint
timer_func(gpointer data)
{
    anim_advance();
    return TRUE;  /* keep going */
}

static void
test_anim_cb(GtkWidget *w, gpointer data)
{
    if (timer < 0)
    {
        int delay = 200; /* millisec */
	timer = gtk_timeout_add(delay, timer_func, 0);
    }
    else
    {
    	gtk_timeout_remove(timer);
	timer = -1;
    }
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
unimplemented(GtkWidget *w, gpointer data)
{
    fprintf(stderr, "Unimplemented\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GtkWidget *
_uiAddMenuAux(
    GtkWidget *menubar,
    const char *label,
    gboolean isRight)
{
    GtkWidget *menu, *item;

    item = gtk_menu_item_new_with_label(label);
    gtk_widget_show(item);

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    gtk_menu_bar_append(GTK_MENU_BAR(menubar), item);
    
    if (isRight)
    	gtk_menu_item_right_justify(GTK_MENU_ITEM(item));

    return menu;
}

static GtkWidget *
uiAddMenu(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, FALSE);
}

static GtkWidget *
uiAddMenuRight(GtkWidget *menubar, const char *label)
{
    return _uiAddMenuAux(menubar, label, TRUE);
}

static GtkWidget *
uiAddButton(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(label);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    gtk_widget_show(item);
    return item;
}

GtkWidget *
uiAddToggle(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata,
    GtkWidget *radioOther,
    gboolean set)
{
    GtkWidget *item;
    
    if (radioOther == 0)
    {
	item = gtk_check_menu_item_new_with_label(label);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), set);
    }
    else
    {
    	GSList *group;
	group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(radioOther));
	item = gtk_radio_menu_item_new_with_label(group, label);
    }
    
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate", 
    	GTK_SIGNAL_FUNC(callback), calldata);
    gtk_widget_show(item);
    return item;
}


static GtkWidget *
uiAddSeparator(GtkWidget *menu)
{
    GtkWidget *item;

    item = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_widget_show(item);
    return item;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
uiCreateMenus(GtkWidget *menubar)
{
    GtkWidget *menu;
    GtkWidget *item;
    
    menu = uiAddMenu(menubar, "File");
    uiAddButton(menu, "Open Log...", unimplemented, 0);
    uiAddButton(menu, "Save Log...", unimplemented, 0);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "Change Makefile...", unimplemented, 0);
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "Change directory...", unimplemented, 0);
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "Print", unimplemented, 0);
    widgets_notempty = g_list_append(widgets_notempty, (gpointer)item);
    uiAddButton(menu, "Print Settings...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Exit", file_exit_cb, 0);
    
    menu = uiAddMenu(menubar, "Build");
    item = uiAddButton(menu, "Again", unimplemented, 0);
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "Stop", unimplemented, 0);
    widgets_running = g_list_append(widgets_running, (gpointer)item);
    uiAddSeparator(menu);
    item = uiAddButton(menu, "all", build_cb, "all");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "install", build_cb, "install");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "clean", build_cb, "clean");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    uiAddSeparator(menu);
#if 1
    item = uiAddButton(menu, "random", build_cb, "random");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "delay", build_cb, "delay");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
    item = uiAddButton(menu, "targets", build_cb, "targets");
    widgets_notrunning = g_list_append(widgets_notrunning, (gpointer)item);
#endif
    
    menu = uiAddMenu(menubar, "View");
    item = uiAddButton(menu, "Clear Log", clear_log_cb, 0);
    widgets_notempty = g_list_append(widgets_notempty, (gpointer)item);
    item = uiAddButton(menu, "Edit Next Error", unimplemented, 0);
    widgets_notempty = g_list_append(widgets_notempty, (gpointer)item);
    item = uiAddButton(menu, "Edit Prev Error", unimplemented, 0);
    widgets_notempty = g_list_append(widgets_notempty, (gpointer)item);
    uiAddSeparator(menu);
    uiAddToggle(menu, "Menubar", show_widget_cb, (gpointer)&menubar, 0, TRUE);
    uiAddToggle(menu, "Toolbar", show_widget_cb, (gpointer)&toolbar, 0, TRUE);
    uiAddToggle(menu, "Messages", show_widget_cb, (gpointer)&messagebox, 0, TRUE);
    uiAddSeparator(menu);
    uiAddButton(menu, "Preferences", unimplemented, 0);
    
    menu = uiAddMenuRight(menubar, "Help");
    uiAddButton(menu, "About Maketool...", unimplemented, 0);
    uiAddButton(menu, "About make...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Help on...", unimplemented, 0);
    uiAddSeparator(menu);
    uiAddButton(menu, "Tutorial", unimplemented, 0);
    uiAddButton(menu, "Reference Index", unimplemented, 0);
    uiAddButton(menu, "Home Page", unimplemented, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "new.xpm"

static void
uiCreateTools()
{
    GtkToolbar *tb = GTK_TOOLBAR(toolbar);
    GdkPixmap *pm;
    GdkBitmap *mask;

    pm = gdk_pixmap_create_from_xpm_d(toplevel->window,
    	&mask, 0, new_xpm);
    gtk_toolbar_append_item(tb,
    	"New", "Create a new file", 0,
    	gtk_pixmap_new(pm, mask), unimplemented, 0);
    gtk_toolbar_append_space(tb);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "anim0.xpm"
#include "anim1.xpm"
#include "anim2.xpm"
#include "anim3.xpm"
#include "anim4.xpm"
#include "anim5.xpm"
#include "anim6.xpm"
#include "anim7.xpm"
#include "anim8.xpm"

#define ANIM_INIT(n) \
    anim_pixmaps[n] = gdk_pixmap_create_from_xpm_d(toplevel->window, \
    	&anim_masks[n], 0, PASTE3(anim,n,_xpm));

static void
uiInitAnimPixmaps(void)
{
    ANIM_INIT(0);
    ANIM_INIT(1);
    ANIM_INIT(2);
    ANIM_INIT(3);
    ANIM_INIT(4);
    ANIM_INIT(5);
    ANIM_INIT(6);
    ANIM_INIT(7);
    ANIM_INIT(8);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
uiCreate(void)
{
    GtkWidget *mainvbox, *vbox; /* need 2 for correct spacing */
    GtkTooltips *tooltips;
    GtkWidget *sb, *logbox;
    
    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(toplevel), "Maketool 0.1");
    gtk_widget_set_usize(toplevel, 600, 500);
    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
    	GTK_SIGNAL_FUNC(file_exit_cb), NULL);
    gtk_container_border_width(GTK_CONTAINER(toplevel), 0);
    gtk_widget_show(GTK_WIDGET(toplevel));
    
    tooltips = gtk_tooltips_new();

    mainvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(toplevel), mainvbox);
    gtk_container_border_width(GTK_CONTAINER(mainvbox), 0);

    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(mainvbox), menubar, FALSE, FALSE, 0);
    uiCreateMenus(menubar);
    gtk_widget_show(GTK_WIDGET(menubar));
        
    vbox = gtk_vbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(mainvbox), vbox, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), SPACING);

    toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    uiCreateTools(toolbar);
    gtk_widget_show(GTK_WIDGET(toolbar));
    
    logbox = gtk_hbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), logbox, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(logbox), 0);
    gtk_widget_show(GTK_WIDGET(logbox));
    
    logwin = gtk_text_new(0, 0);
    gtk_text_set_editable(GTK_TEXT(logwin), FALSE);
    gtk_box_pack_start(GTK_BOX(logbox), logwin, TRUE, TRUE, 0);   
    gtk_widget_show(logwin);
    
    sb = gtk_vscrollbar_new(GTK_TEXT(logwin)->vadj);
    gtk_box_pack_start(GTK_BOX(logbox), sb, FALSE, FALSE, 0);   
    gtk_widget_show(sb);
    
    messagebox = gtk_hbox_new(FALSE, SPACING);
    gtk_box_pack_start(GTK_BOX(vbox), messagebox, FALSE, FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(messagebox), 0);
    gtk_widget_show(GTK_WIDGET(messagebox));
    
    messageent = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(messageent), FALSE);
    gtk_entry_set_text(GTK_ENTRY(messageent), "Welcome to Maketool");
    gtk_box_pack_start(GTK_BOX(messagebox), messageent, TRUE, TRUE, 0);   
    gtk_widget_show(messageent);
    
    uiInitAnimPixmaps();

    anim = gtk_pixmap_new(anim_pixmaps[0], anim_masks[0]);
    gtk_box_pack_start(GTK_BOX(messagebox), anim, FALSE, FALSE, 0);   
    gtk_widget_show(anim);

    grey_menu_items();

    gtk_widget_show(GTK_WIDGET(mainvbox));
    gtk_widget_show(GTK_WIDGET(vbox));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [options] target [target...]\n", argv0);
    /* TODO: options */
}

static void
parseArgs(int argc, char **argv)
{
    int i;
    
    argv0 = argv[0];
    
    numTargets = 0;
    currentTarget = 0;
    targets = (char**)malloc(sizeof(char*) * argc);
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    /* TODO: options */
	    usage();
	}
	else
	{
	    targets[numTargets++] = argv[i];
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    parseArgs(argc, argv);
    uiCreate();
    gtk_main();
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
