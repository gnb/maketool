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

#include "common.h"
#include "glib_extra.h"
#include <signal.h>
#include <sys/poll.h>

CVSID("$Id: glib_extra.c,v 1.11 2000-01-03 14:47:14 gnb Exp $");


typedef struct
{
    pid_t pid;
    GUnixReapFunc reaper;
    gpointer user_data;
} GPidData;

static GHashTable *g_unix_piddata = 0;
static gboolean g_unix_got_signal = FALSE;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
g_unix_default_reaper(pid_t pid, int status, struct rusage *usg, gpointer data)
{
#if DEBUG
    if (WIFEXITED(status) || WIFSIGNALED(status))
    	fprintf(stderr, "g_unix_default_reaper: reaped pid %d\n", (int)pid);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
g_unix_dispatch_reapers(void)
{
    GPidData *pd;
    pid_t pid;
    int status = 0;
    struct rusage usage;
    
#if DEBUG
    fprintf(stderr, "g_unix_dispatch_reapers()\n");
#endif
    
    for (;;)
    {
#if HAVE_WAIT3    
	pid = wait3(&status, WNOHANG, &usage);
#else
	memset(&usage, 0, sizeof(usage));	/* nothing to see here */
	pid = waitpid(-1, &status, WNOHANG);
#endif
	
#if DEBUG
	fprintf(stderr, "g_unix_check_processes(): pid = %d\n", (int)pid);
#endif
	if (pid <= 0)
    	    break;

	pd = (GPidData *)g_hash_table_lookup(g_unix_piddata, GINT_TO_POINTER(pid));
	if (pd != 0)
	{
	    (*pd->reaper)(pid, status, &usage, pd->user_data);
	    if (WIFEXITED(status) || WIFSIGNALED(status))
	    	g_hash_table_remove(g_unix_piddata, GINT_TO_POINTER(pid));
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static RETSIGTYPE
g_unix_signal_handler(int sig)
{
#if DEBUG > 50
    static const char msg[] = "g_unix_signal_handler(): called\n";
    write(2, msg, sizeof(msg)-1);
#endif
    g_unix_got_signal = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static gboolean
g_unix_source_prepare(
    gpointer source_data, 
    GTimeVal *current_time,
    gint *timeout,
    gpointer user_data)
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_prepare(): called\n");
#endif
    return g_unix_got_signal;
}
    
static gboolean
g_unix_source_check(
    gpointer source_data,
    GTimeVal *current_time,
    gpointer user_data)
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_check(): called\n");
#endif
    return g_unix_got_signal;
}
    
static gboolean
g_unix_source_dispatch(
    gpointer source_data, 
    GTimeVal *current_time,
    gpointer user_data)
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_dispatch(): called\n");
#endif
    g_unix_got_signal = FALSE;
    g_unix_dispatch_reapers();
    return TRUE;
}
    
static void
g_unix_source_destroy(gpointer data)
{
#if DEBUG
    fprintf(stderr, "g_unix_source_destroy(): called\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GSourceFuncs reaper_source_funcs = 
{
g_unix_source_prepare,
g_unix_source_check,
g_unix_source_dispatch,
g_unix_source_destroy
};

/*
 * TODO: there is some bizarre subtle problem with either my code
 * or one of gtk or gdk which causes strange display bugs when
 * this priority is made equal to 1. This needs to be properly
 * tracked down and exterminated, but for the time being making
 * the priority equal to G_PRIORITY_DEFAULT works -- Greg, 3Jan2000.
 */
#define G_UNIX_REAP_PRIORITY	G_PRIORITY_DEFAULT

void
g_unix_reap_init(void)
{
    static gboolean first = TRUE;
    
    if (first)
    {
    	first = FALSE;
	g_source_add(
		G_UNIX_REAP_PRIORITY,	/* priority */
		TRUE,			/* can_recurse */
		&reaper_source_funcs,
		(gpointer)0,		/* source_data */
		(gpointer)0,		/* user_data */
		(GDestroyNotify)0);
    	g_unix_piddata = g_hash_table_new(g_direct_hash, g_direct_equal);
	signal(SIGCHLD, g_unix_signal_handler);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*TODO:return a gint tag for removal*/

void
g_unix_add_reap_func(
    pid_t pid,
    GUnixReapFunc reaper,
    gpointer user_data)
{
    GPidData *pd;
    
    if (reaper == 0)
    	reaper = g_unix_default_reaper;

    pd = g_new(GPidData, 1);
    pd->pid = pid;
    pd->reaper = reaper;
    pd->user_data = user_data;
    g_hash_table_insert(g_unix_piddata, GINT_TO_POINTER(pid), (gpointer)pd);
    
#if DEBUG
    fprintf(stderr, "g_unix_add_reaper(): pid = %d\n", (int)pid);
#endif
}

/* TODO:
void
g_unix_remove_reap_func(gint tag)
*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
