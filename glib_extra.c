#define DEBUG 0

#include "common.h"
#include "glib_extra.h"
#include <signal.h>
#include <sys/poll.h>

CVSID("$Id: glib_extra.c,v 1.5 1999-05-25 08:02:47 gnb Exp $");


typedef struct
{
    pid_t pid;
    GUnixReapFunc reaper;
    gpointer user_data;
} GPidData;

static GHashTable *g_unix_piddata = 0;
static int g_unix_got_signal = 0;


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

static gboolean
g_unix_dispatch_reapers(gpointer data)
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
	pid = wait3(&status, WNOHANG, &usage);
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
    return FALSE;	/* stop calling this function */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: copy #ifndef HAVE_POLL definition of g_poll() from glib's gmain.c */

static gint
g_unix_poll_func(
    GPollFD *ufds,
    guint nfds,
    gint timeout)
{
    int ret;
    
    ret = poll((struct pollfd*)ufds, nfds, timeout);
    
#if DEBUG > 10
    fprintf(stderr, "g_unix_poll_func(): ret=%d errno=%d g_unix_got_signal=%d\n",
    	ret, errno, g_unix_got_signal);
#endif

    if (g_unix_got_signal > 0)
    {
#if DEBUG
	fprintf(stderr, "g_unix_poll_func(): g_unix_got_signal=%d\n", g_unix_got_signal);
#endif
    	g_idle_add(g_unix_dispatch_reapers, (gpointer)0);
    	g_unix_got_signal = 0;
    }
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
g_unix_signal_handler(int sig)
{
    g_unix_got_signal++;
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
    static gboolean first = TRUE;
    
    if (first)
    {
    	first = FALSE;
	g_main_set_poll_func(g_unix_poll_func);
    	g_unix_piddata = g_hash_table_new(g_direct_hash, g_direct_equal);
	signal(SIGCHLD, g_unix_signal_handler);
    }
    
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
