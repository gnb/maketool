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

#ifndef _TASK_H_
#define _TASK_H_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct _Task Task;
typedef struct _TaskOps TaskOps;

struct _Task
{
    char *command;
    char **environ;
    pid_t pid;
    int fd;
    int input_tag;
    gboolean enqueued;
    TaskOps *ops;
};

struct _TaskOps
{    
    void (*start)(Task*);   	/* called when command spawned */
    void (*input)(Task*, int len, const char *buf);
    	    	    	    	/* called when command emits output */
    void (*reap)(Task*);    	/* called when command ends */
    void (*destroy)(Task*);
    	    	    	    	/* called to clean up Task object */
};


/*
 * Initialise the task package. The two given functions
 * are called when the first enqueued task is spawned,
 * and the last enqueued task is reaped resp; they can
 * be used to show a `working' indication to the user.
 */
void task_init(
    void (*work_start_cb)(void),
    void (*work_end_cb)(void));

/*
 * Equivalent of an inherited c'tor -- initialises the Task struct.
 * `command' is assumed to be a new string which is g_free()d later.
 * `env' is *not* g_free()d -- WARNING.
 */
Task *task_create(Task *, char *command, char **env, TaskOps *);

/*
 * Enqueue the task for execution *after* all currently enqueued tasks.
 */
void task_enqueue(Task *);


/*
 * Begin execution of any enqueued tasks
 */
void task_start(void);

/*
 * Cause the immediate spawning of an un-enqueued task.
 */
gboolean task_spawn(Task *);

/*
 * Returns TRUE iff a task is currently running.
 */
gboolean task_is_running(void);

/*
 * Kill the currently running task.
 */
void task_kill_current(void);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _TASK_H_ */