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

#include "maketool_task.h"
#include <signal.h>
#include <gdk/gdk.h>
#include "glib_extra.h"

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_SYS_FILIO_H
/* for FIONREAD on Solaris */
#include <sys/filio.h>
#endif

CVSID("$Id: task.c,v 1.16 2003-09-27 13:36:42 gnb Exp $");

/*
 * TODO: GDK is used only for the gdk_input_*() functions, which
 * are not immensely complicated and could be reproduced in here
 * if this library were to be submitted to the glib maintainers.
 */
 
static GList *task_all = 0;
static void (*task_work_start_cb)(void) = 0;
static void (*task_work_end_cb)(void) = 0;
static void task_input_func(gpointer user_data, gint source, GdkInputCondition condition);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_init(
    void (*work_start_cb)(void),
    void (*work_end_cb)(void))
{
    g_unix_reap_init();
    task_work_start_cb = work_start_cb;
    task_work_end_cb = work_end_cb;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

Task *
task_create(Task *task, char *command, char **env, TaskOps *ops, int flags)
{
    task->refcount = 1;
    task->pid = -1;
    task->command = command;
    task->environ = env;
    task->fd = -1;
    task->input_tag = 0;
    task->enqueued = FALSE;
    task->ops = ops;
    task->flags = flags;
    estring_init(&task->linebuf);
    return task;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
task_enqueue(Task *task)
{
    task->enqueued = TRUE;
    task_all = g_list_append(task_all, task);
}

static void
task_destroy(Task *task)
{
    g_free(task->command);
    if (task->fd != -1)
    {
    	close(task->fd);
	gdk_input_remove(task->input_tag);
    }
    if (task->ops->destroy != 0)
	(*task->ops->destroy)(task);
    estring_free(&task->linebuf);
    g_free(task);
}


void
task_ref(Task *task)
{
    task->refcount++;
}

void
task_unref(Task *task)
{
    if (--task->refcount == 0)
    	task_destroy(task);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * This used to be main.c:handle_input().  It splits input
 * from the child process's stderr/stderr into lines and
 * feeds individual lines to the task input function.
 */
static void
task_linemode_input(Task *task, int len, const char *buf)
{
    if (len == 0)
    {
    	/* end of input */
	/*
	 * Handle case where last line of child process'
	 * output has no terminating '\n'. Beware - 
	 * this last line may contain an error or warning,
	 * which affects log_num_{errors,warnings}().
	 */
	if (task->linebuf.length > 0)
	    (*task->ops->input)(task, task->linebuf.length, task->linebuf.data);
	return;
    }
    
    /* TODO: use append_chars here to deal with NULs in input stream */

    while (len > 0 && *buf)
    {
	char *p = strchr(buf, '\n');
	if (p == 0)
	{
    	    /* only a part of a line left - append to task->linebuf */
	    estring_append_string(&task->linebuf, buf);
	    return;
	}
    	/* got an end-of-line - isolate the line & feed it to input handler */
	*p = '\0';
	estring_append_string(&task->linebuf, buf);

    	if (task->linebuf.length > 0)
	    (*task->ops->input)(task, task->linebuf.length, task->linebuf.data);
    	else
	    (*task->ops->input)(task, 0, "");
	
	estring_truncate(&task->linebuf);
	len -= (p - buf);
	buf = ++p;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
task_reap_func(pid_t pid, int status, struct rusage *usg, gpointer user_data)
{
    Task *task = (Task *)user_data;
    gboolean was_enqueued = task->enqueued;
    
    if (!(WIFEXITED(status) || WIFSIGNALED(status)))
    	return;
#if DEBUG
    fprintf(stderr, "reaped \"%s\", pid %d\n", task->command, (int)pid);
#endif
    if (task->pid != pid)
    {
    	/* it's not paranoia when they're really out to get you */
    	fprintf(stderr, _("maketool: reaped unexpected pid %d\n"), (int)pid);
    }
    
    /* Hack to cause any pending input from the pipe to
     * be processed before the reap function tries to use it.
     */
    if (task->fd > 0)
	task_input_func(user_data, task->fd, GDK_INPUT_READ);

    task->pid = -1; 	/* so task_is_running() == FALSE in reap fn */
    task_all = g_list_remove(task_all, task);
    
    task->status = status;
    /* flush any pending lines or part-lines */
    if (task->flags & TASK_LINEMODE)
    	task_linemode_input(task, 0, 0);
    if (task->ops->reap != 0)
	(*task->ops->reap)(task);
    task_unref(task);
    
    if (was_enqueued)
    {
    	if (task_all == 0)
    	{
	    if (task_work_end_cb != 0)
	    	(*task_work_end_cb)();
	}
	else
    	    task_start();	/* spawn the next task in the queue */
    }
}

gboolean
task_is_successful(const Task *task)
{
    return (WIFEXITED(task->status) && WEXITSTATUS(task->status) == 0);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
task_input_func(gpointer user_data, gint source, GdkInputCondition condition)
{
    Task *task = (Task *)user_data;
    int nremain = 0;
    
    if (source != task->fd || condition != GDK_INPUT_READ)
    	return;
	
    if (ioctl(task->fd, FIONREAD, &nremain) < 0)
    {
	perror("ioctl(FIONREAD)");
	return;
    }

    while (nremain > 0)
    {
	char buf[1025];
	int n = read(task->fd, buf, MIN((int)sizeof(buf)-1, nremain));
	nremain -= n;
	buf[n] = '\0';		/* so we can use str*() calls */

#if DEBUG > 40
	fprintf(stderr, "task_input_func(): \"");
	fwrite(buf, n, 1, stderr);
	fprintf(stderr, "\"\n");
#endif    

    	if (task->flags & TASK_LINEMODE)
	    task_linemode_input(task, n, buf);
	else
	    (*task->ops->input)(task, n, buf);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
task_override_env(Task *task)
{
#if HAVE_PUTENV
    char **e;
    if ((e = task->environ) != 0)
    {
	/* TODO: do a merge of the current env & Variables
	 *       in preferences.c when changes are applied.
	 *       That's more efficient but more complex, 'cos
	 *       here putenv() handles the uniquifying for us.
	 */
	for ( ; *e != 0 ; e++)
	    putenv(*e);
    }
#else
#warning putenv not defined - some functionality will be missing
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static pid_t
task_spawn_simple(Task *task)
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
	task_override_env(task);

	/* Possibly become process group leader */
	if (task->flags & TASK_GROUPLEADER)
	    setpgid(0, 0);

	execlp("/bin/sh", "/bin/sh", "-c", task->command, 0);
	perror("execlp");
	exit(1);
    }
    else
    {
    	/* parent */
    }
    return pid;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define READ 0
#define WRITE 1
#define STDOUT 1
#define STDERR 2

static pid_t
task_spawn_with_input(Task *task)
{
    pid_t pid;
    int pipefds[2];
    
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
	
	task_override_env(task);

	/* Possibly become process group leader */
	if (task->flags & TASK_GROUPLEADER)
	    setpgid(0, 0);

	execl("/bin/sh", "/bin/sh", "-c", task->command, 0);
	perror("execl");
	exit(1);
    }
    else
    {
    	/* parent */
	close(pipefds[WRITE]);
	
	task->fd = pipefds[READ];
	task->input_tag = gdk_input_add(task->fd,
			    GDK_INPUT_READ, task_input_func, (gpointer)task);
    }
    return pid;
}

#undef READ
#undef WRITE
#undef STDOUT
#undef STDERR

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_spawn(Task *task)
{
    assert(task->command != 0);
    if (task->ops->input == 0)
	task->pid = task_spawn_simple(task);
    else
	task->pid = task_spawn_with_input(task);

#if DEBUG
    fprintf(stderr, "spawned \"%s\", pid %d\n", task->command, (int)task->pid);
#endif

    if (task->pid > 0)
	g_unix_add_reap_func(task->pid, task_reap_func, (gpointer)task);
	
    if (task->ops->start != 0)
    	(*task->ops->start)(task);

    return (task->pid != -1);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
task_run(Task *task)
{
    if (!task_spawn(task))
    	return FALSE;
    
    task_ref(task);    
    while (task->pid != -1)
    	g_main_iteration(/*block*/TRUE);
    task_unref(task);
	
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static Task *
task_current(void)
{
    Task *curr;
    
    if (task_all == 0)
    	return 0;
    curr = (Task *)task_all->data;
    return (curr->pid == -1 ? 0 : curr);
}

void
task_start(void)
{
    Task *curr = task_current();
    
    if (curr == 0 &&
    	task_all != 0)
    {
    	if (task_spawn((Task *)task_all->data))
	{
    	    if (task_work_start_cb != 0)
		(*task_work_start_cb)();
	}
    }
}

gboolean
task_is_running(void)
{
    return (task_current() != 0);
}

void
task_kill_current(void)
{
    Task *curr = task_current();
    
    if (curr != 0)
    	kill(curr->pid, SIGKILL);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
