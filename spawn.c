#include "spawn.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/poll.h>

#define DEBUG 0

typedef struct
{
    pid_t pid;
    void (*reaper)(pid_t, int status, struct rusage*, gpointer);
    gpointer user_data;
} PidData;

static GHashTable *spawn_piddata = 0;
static int spawn_got_signal = 0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
spawn_dispatch_reapers(gpointer data)
{
    PidData *pd;
    pid_t pid;
    int status = 0;
    struct rusage usage;
    
    fprintf(stderr, "spawn_dispatch_reapers()\n");
    
    for (;;)
    {
	pid = wait3(&status, WNOHANG, &usage);
	fprintf(stderr, "spawn_check_processes(): pid = %d\n", (int)pid);
	if (pid <= 0)
    	    break;

	pd = (PidData *)g_hash_table_lookup(spawn_piddata, GINT_TO_POINTER(pid));
	if (pd != 0)
	{
	    (*pd->reaper)(pid, status, &usage, pd->user_data);
	    if (WIFEXITED(status) || WIFSIGNALED(status))
	    	g_hash_table_remove(spawn_piddata, GINT_TO_POINTER(pid));
	}
    }
    return FALSE;	/* stop calling this function */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: copy #ifndef HAVE_POLL definition of g_poll() from glib's gmain.c */

static gint
spawn_poll_func(
    GPollFD *ufds,
    guint nfds,
    gint timeout)
{
    int ret;
    
    ret = poll((struct pollfd*)ufds, nfds, timeout);
    
#if DEBUG
    fprintf(stderr, "spawn_poll_func(): ret=%d errno=%d spawn_got_signal=%d\n",
    	ret, errno, spawn_got_signal);
#endif

    if (spawn_got_signal > 0)
    {
    	g_idle_add(spawn_dispatch_reapers, (gpointer)0);
    	spawn_got_signal = 0;
    }
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
spawn_signal_handler(int sig)
{
    spawn_got_signal++;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*TODO:return a gint tag for removal*/
void
spawn_add_reap_func(
    pid_t pid,
    void (*reaper)(pid_t, int status, struct rusage *, gpointer),
    gpointer user_data)
{
    PidData *pd;
    static gboolean first = TRUE;
    
    if (first)
    {
    	first = FALSE;
	g_main_set_poll_func(spawn_poll_func);
    	spawn_piddata = g_hash_table_new(g_direct_hash, g_direct_equal);
	signal(SIGCHLD, spawn_signal_handler);
    }
    
    pd = g_new(PidData, 1);
    pd->pid = pid;
    pd->reaper = reaper;
    pd->user_data = user_data;
    g_hash_table_insert(spawn_piddata, GINT_TO_POINTER(pid), (gpointer)pd);
    
    fprintf(stderr, "spawn_add_reaper(): pid = %d\n", (int)pid);
}

/* TODO:
void
spawn_remove_reap_func(gint tag)
*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
