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

#include "mqueue.h"
#include <gdk/gdkx.h>

CVSID("$Id: client.c,v 1.1 2001-09-21 04:33:04 gnb Exp $");

char *argv0;
GdkWindow *command_window;  /* maketool ancestor's main window */
GdkWindow *reply_window;    /* our own invisible window for reply */
estring command = ESTRING_STATIC_INIT;
long result = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Create an invisible window for the sole purpose of
 * receiving a message reply.
 */

static GdkWindow *
create_reply_window(void)
{
    GdkWindowAttr wattr;
    gint wmask;

    memset(&wattr, 0, sizeof(wattr));
    wmask = 0;
    
    wattr.title = "Maketool Command Reply"; wmask |= GDK_WA_TITLE;
    wattr.event_mask = GDK_PROPERTY_CHANGE_MASK;
    wattr.x = 0; wmask |= GDK_WA_X;
    wattr.y = 0; wmask |= GDK_WA_Y;
    wattr.width = 1;
    wattr.height = 1;
    wattr.wclass = GDK_INPUT_ONLY;
    wattr.override_redirect = TRUE;  wmask |= GDK_WA_NOREDIR;
    
    return gdk_window_new(GDK_ROOT_PARENT(), &wattr, wmask);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int got_event;

static void
event_handler(GdkEvent *event, gpointer	data)
{
    Message *msg = (Message *)data;
    
    if (mq_reply_event(event) && mq_get_reply(msg, &result))
    {
	got_event++;
	mq_msg_delete(msg);
    }
    /*gdk_event_free(event);*/
}

static void
send_command(void)
{
    Message *msg;

    mq_init(reply_window);
    
    msg = mq_msg_new_e(&command);
    mq_send(command_window, msg);

    gdk_event_handler_set(event_handler, /*data*/msg, /*notify*/0);

    do
	g_main_iteration(/*may_block*/TRUE);
    while (!got_event);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_string[] = "\
Usage: maketool_client [--window wid] command ...\n\
";

static void
usage(int code)
{
    fputs(usage_string, stderr);
    exit(code);
}


static void
parse_args(int argc, char **argv)
{
    int i;
    Window xwindow = 0;
    
    argv0 = argv[0];
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "--window"))
	    {
	    	if (i == argc-1)
		    usage(1);
	    	if ((xwindow = strtol(argv[++i], 0, 0)) == 0)
		    usage(1);
	    }
	    else
	    {
		usage(1);
	    }
	}
	else
	{
	    if (command.length > 0)
		estring_append_char(&command, ' ');
	    estring_append_string(&command, argv[i]);
	}
    }
    
    if (xwindow == 0)
    {
    	char *env = getenv("MAKETOOL_WINDOWID");

#if DEBUG
    	fprintf(stderr, "maketool_client: MAKETOOL_WINDOWID=%s\n",
	    (env == 0 ? "<null>" : env));
#endif
	
	if (env == 0 || (xwindow = strtol(env, 0, 0)) == 0)
	{
	    fprintf(stderr,
	    	"%s: can't get window id from --window argument or $MAKETOOL_WINDOWID, exiting\n",
		argv0);
	    exit(1);
	}
    }
    assert(xwindow != 0);
    command_window = gdk_window_foreign_new(xwindow);
    reply_window = create_reply_window();    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    gdk_init(&argc, &argv);    
    parse_args(argc, argv);
    send_command();
    return result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
