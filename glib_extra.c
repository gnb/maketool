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

#include "common.h"
#include "glib_extra.h"
#include <signal.h>
#include <sys/poll.h>

CVSID("$Id: glib_extra.c,v 1.20 2003-10-29 12:39:18 gnb Exp $");


typedef struct
{
    pid_t pid;
    GUnixReapFunc reaper;
    gpointer user_data;
} GPidData;

static GHashTable *g_unix_piddata = 0;
static gboolean g_unix_got_signal = FALSE;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Handle registering signals.  Ideally we want BSD signal semantics
 * where we can register a handler once, it's suppressed during
 * delivery so we can't get recursive delivery, and it's never
 * automatically un-registered.  On some OSes we get that for free,
 * on some OSes we need to perform a little dance with sigaction()
 * or sigvec() to achieve that; on yet other OSes we only get the
 * screwed up SysV signal semantics.  Maketool will probably cause
 * recursive signal delivery and a stack blowout on those platforms.
 *
 * register_sighandler
 *  	Called to register the signal handler the first time only
 *
 * reregister_sighandler
 *  	Called in the signal handler to re-register the signal
 *  	handler if necessary (in the ideal case this is empty).
 */

#if SIGNAL_SEMANTICS == SIGNAL_SEMANTICS_SIGACTION

static void
register_sighandler(int sig, RETSIGTYPE (*handler)(int sig))
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
#ifdef SA_RESTART    
    act.sa_flags |= SA_RESTART;
#endif
    sigaction(sig, &act, 0);
}
#define reregister_sighandler(sig, handler)

#elif SIGNAL_SEMANTICS == SIGNAL_SEMANTICS_SIGVEC

static void
register_sighandler(int sig, RETSIGTYPE (*handler)(int sig))
{
    struct sigvec vec;

    memset(&vec, 0, sizeof(vec));
    vec.sv_handler = handler;
    sigvec(sig, &vec, 0);
}
#define reregister_sighandler(sig, handler)

#elif SIGNAL_SEMANTICS == SIGNAL_SEMANTICS_SYSV

#define register_sighandler(sig, handler) \
	signal(sig, handler)
#define reregister_sighandler(sig, handler) \
    	register_sighandler(sig, handler)

#elif SIGNAL_SEMANTICS == SIGNAL_SEMANTICS_BSD

#define register_sighandler(sig, handler) \
	signal(sig, handler)
#define reregister_sighandler(sig, handler)

#endif /* SIGNAL_SEMANTICS_BSD */

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
    	int flags = WNOHANG;

#if HAVE_BSD_JOB_CONTROL
    	flags |= WUNTRACED;
#endif

#if HAVE_WAIT3    
	pid = wait3(&status, flags, &usage);
#else
	memset(&usage, 0, sizeof(usage));	/* nothing to see here */
	pid = waitpid(-1, &status, flags);
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
	    {
	    	g_hash_table_remove(g_unix_piddata, GINT_TO_POINTER(pid));
		g_free(pd);
	    }
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

    reregister_sighandler(SIGCHLD, g_unix_signal_handler);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static gboolean
g_unix_source_prepare(
#if GLIB2
    GSource *source,
    gint *timeout
#else
    gpointer source_data, 
    GTimeVal *current_time,
    gint *timeout,
    gpointer user_data
#endif
    )
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_prepare(): returning %d\n", (int)g_unix_got_signal);
#endif
    return g_unix_got_signal;
}
    
static gboolean
g_unix_source_check(
#if GLIB2
    GSource *source
#else
    gpointer source_data,
    GTimeVal *current_time,
    gpointer user_data
#endif
    )
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_check(): returning %d\n", (int)g_unix_got_signal);
#endif
    return g_unix_got_signal;
}
    
static gboolean
g_unix_source_dispatch(
#if GLIB2
    GSource *source,
    GSourceFunc callback,
    gpointer user_data
#else
    gpointer source_data, 
    GTimeVal *current_time,
    gpointer user_data
#endif
    )
{
#if DEBUG > 5
    fprintf(stderr, "g_unix_source_dispatch(): called\n");
#endif
    g_unix_got_signal = FALSE;
    g_unix_dispatch_reapers();
    return TRUE;
}
    
static void
g_unix_source_destroy(
#if GLIB2
    GSource *source
#else
    gpointer data
#endif
    )
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
#if GLIB2
	GSource *source;
#endif

    	first = FALSE;
	register_sighandler(SIGCHLD, g_unix_signal_handler);
    	g_unix_piddata = g_hash_table_new(g_direct_hash, g_direct_equal);
#if GLIB2
    	source = g_source_new(&reaper_source_funcs, sizeof(GSource));
	g_source_set_can_recurse(source, TRUE);
    	g_source_set_priority(source, G_UNIX_REAP_PRIORITY);
	g_source_attach(source, g_main_context_default());
#else
	g_source_add(
		G_UNIX_REAP_PRIORITY,	/* priority */
		TRUE,			/* can_recurse */
		&reaper_source_funcs,
		(gpointer)0,		/* source_data */
		(gpointer)0,		/* user_data */
		(GDestroyNotify)0);
#endif
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
