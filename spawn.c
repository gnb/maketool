#include "spawn.h"
#include <signal.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>

#define DEBUG 1

CVSID("$Id: spawn.c,v 1.6 1999-05-25 08:02:48 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

pid_t
spawn_simple(
    const char *command,
    GUnixReapFunc reaper,
    gpointer reaper_data)
{
    pid_t pid;
    
    if ((pid = fork()) < 0)
    {
    	/* error */
    	perror("fork");
	return -1;
    }
    else if (pid == 0)
    {
    	/* child */
	execlp("/bin/sh", "/bin/sh", "-c", command, 0);
	perror("execlp");
	exit(1);
    }
    else
    {
    	/* parent */
	g_unix_add_reap_func(pid, reaper, reaper_data);
    }
    return pid;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    GUnixReapFunc reap_func;
    SpawnInputFunc input_func;
    gpointer user_data;
    int fd;
    int input_tag;
} SpawnData;

static void
spawn_output_reaper(pid_t pid, int status, struct rusage *usg, gpointer data)
{
    SpawnData *so = (SpawnData *)data;
    
    if (so->reap_func != 0)
	(*so->reap_func)(pid, status, usg, so->user_data);

    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
    	close(so->fd);
	gtk_input_remove(so->input_tag);
    	g_free(so);
    }
}

static void
spawn_output_input(gpointer data, gint source, GdkInputCondition condition)
{
    SpawnData *so = (SpawnData *)data;
    int nremain = 0;
    
    if (source != so->fd || condition != GDK_INPUT_READ)
    	return;
	
    if (ioctl(so->fd, FIONREAD, &nremain) < 0)
    {
	perror("ioctl(FIONREAD)");
	return;
    }

    while (nremain > 0)
    {
	char buf[1025];
	int n = read(so->fd, buf, MIN(sizeof(buf)-1, nremain));
	nremain -= n;
	buf[n] = '\0';		/* so we can use str*() calls */
	(*so->input_func)(n, buf, so->user_data);
    }
}

#define READ 0
#define WRITE 1
#define STDOUT 1
#define STDERR 2

pid_t
spawn_with_output(
    const char *command,
    GUnixReapFunc reaper,
    SpawnInputFunc input,	/* called with data from child's std{err,out} */
    gpointer user_data,
    char **environ)		/* may be 0 */
{
    pid_t pid;
    int pipefds[2];
    SpawnData *so;
    
    if (pipe(pipefds) < 0)
    {
    	perror("pipe");
    	return -1;
    }
    
    if ((pid = fork()) < 0)
    {
    	/* error */
    	perror("fork");
	return -1;
    }
    else if (pid == 0)
    {
    	/* child */
	close(pipefds[READ]);
	dup2(pipefds[WRITE], STDOUT);
	dup2(pipefds[WRITE], STDERR);
	close(pipefds[WRITE]);
	
	if (environ != 0)
	{
	    /* TODO: do a merge of the current env & Variables
	     *       in preferences.c when changes are applied.
	     *       That's more efficient but more complex, 'cos
	     *       here putenv() handles the uniquifying for us.
	     */
	    for ( ; *environ != 0 ; environ++)
	    	putenv(*environ);
	}

	execl("/bin/sh", "/bin/sh", "-c", command, 0);
	perror("execl");
	exit(1);
    }
    else
    {
    	/* parent */
	close(pipefds[WRITE]);
	
	so = g_new(SpawnData, 1);
	so->reap_func = reaper;
	so->input_func = input;
	so->user_data = user_data;
	so->fd = pipefds[READ];
	so->input_tag = gdk_input_add(so->fd,
			    GDK_INPUT_READ, spawn_output_input, (gpointer)so);
	g_unix_add_reap_func(pid, spawn_output_reaper, (gpointer)so);
    }
    return pid;
}
 
#undef READ
#undef WRITE
#undef STDOUT
#undef STDERR


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
