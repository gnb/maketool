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


#include "maketool.h"
#if HAVE_SUN_MAKE
#include "util.h"
#if HAVE_REGCOMP
#include <regex.h>	/* POSIX regular expression fns */
#else
#error Why are you even bothering to compile with POSIX regular expressions?
#endif

CVSID("$Id: sunmake.c,v 1.1 2003-05-04 04:28:21 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_makefile_flags(estring *e)
{
    if (prefs.makefile != 0)
    {
    	estring_append_string(e, "-f ");
	estring_append_string(e, prefs.makefile);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_parallel_flags(estring *e)
{
    /* Sun make does not implement parallelism */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_keep_going_flags(estring *e)
{
    if (prefs.keep_going)
    	estring_append_string(e, "-k");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_dryrun_flags(estring *e)
{
    if (prefs.dryrun)
	estring_append_string(e, "-n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_version_flags(estring *e)
{
    estring_append_string(e, "-V"); 	/* TODO: check */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sunmake_list_targets_flags(estring *e)
{
    estring_append_string(e, "-n -p _no_such_target_");
}

typedef struct
{
    Task task;
#define NUM_ERROR_LINES 16
    int nlines;
    estring error_buf;	    /* first NUM_ERROR_LINES of output */
    GPtrArray *targets;
    regex_t target_re;
} SunmakeListTargetsTask;

static void
sunmake_list_targets_reap(Task *task)
{
    SunmakeListTargetsTask *ltt = (SunmakeListTargetsTask *)task;

    if (!task_is_successful(task))
    {
    	/* Report error to user */
    	list_targets_error(ltt->error_buf.data);
    }
    else
    {
    	/* Set all the targets */
	set_targets(ltt->targets->len, (char **)ltt->targets->pdata);

	/* set_targets() has taken over the array and strings, so forget them */
	g_ptr_array_free(ltt->targets, /*free_seg*/FALSE);
	ltt->targets = 0;
    }
}

/*
 * Task has TASK_LINEMODE flag, so the input function gets
 * called once per line.  Unlike several UNIX awk implementations
 * we have no line length limit.
 */

static void
sunmake_list_targets_input(Task *task, int len, const char *buf)
{
    SunmakeListTargetsTask *ltt = (SunmakeListTargetsTask *)task;
    regmatch_t match[2];

    /*
     * Save the first few lines of the output to a buffer
     * in case they represent an error message.
     */
    if (++ltt->nlines <= NUM_ERROR_LINES)
    {
    	estring_append_chars(&ltt->error_buf, buf, len);
	estring_append_char(&ltt->error_buf, '\n');
    }

    /* Match line against the target regular expression. */
    if (regexec(&ltt->target_re, buf, /*nmatches*/2, match, 0) == 0)
    {
	/* extract target name from line...steal the buffer temporarily */
	char *targ, oldc;
	int len;

	targ = (char *)buf+match[1].rm_so;
	len = match[1].rm_eo - match[1].rm_so;

	oldc = targ[len];
	targ[len] = '\0';

	if (filter_target(targ))
	    g_ptr_array_add(ltt->targets, g_strdup(targ));

	targ[len] = oldc;
    }
}


static void
sunmake_list_targets_destroy(Task *task)
{
    SunmakeListTargetsTask *ltt = (SunmakeListTargetsTask *)task;

    if (ltt->targets != 0)
    {
    	int i;
	
	for (i = 0 ; i < ltt->targets->len ; i++)
	    g_free(ltt->targets->pdata[i]);
	g_ptr_array_free(ltt->targets, /*free_seg*/TRUE);
    }

    regfree(&ltt->target_re);
    estring_free(&ltt->error_buf);
}

static TaskOps sunmake_list_targets_ops =
{
0,  	    	    	    	/* start */
sunmake_list_targets_input,	/* input */
sunmake_list_targets_reap, 	/* reap */
sunmake_list_targets_destroy	/* destroy */
};


static Task *
sunmake_list_targets_task(char *cmd)
{
    SunmakeListTargetsTask *ltt = g_new(SunmakeListTargetsTask, 1);
    int err;

    ltt->targets = g_ptr_array_new();
    err = regcomp(&ltt->target_re, "^([^#:=! \t]+):", REG_EXTENDED);
    assert(err == 0);
    ltt->nlines = 0;
    estring_init(&ltt->error_buf);
    
    return task_create(
    	(Task *)ltt,
	/*cmd*/strdup("cat sunmake.txt"),
	prefs.var_environment,
	&sunmake_list_targets_ops,
	TASK_LINEMODE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char * const sunmake_makefiles[] = 
{
    "Makefile",
    "makefile",
    0
};


const MakeProgram makeprog_sun_make = 
{
    "sunmake",
    "make",
    N_("Sun make"),
    /*logo_xpm*/0,
    sunmake_makefiles,
    sunmake_makefile_flags,
    sunmake_parallel_flags,
    sunmake_keep_going_flags,
    sunmake_dryrun_flags,
    sunmake_version_flags,
    sunmake_list_targets_flags,
    sunmake_list_targets_task
};

#endif /* HAVE_SUN_MAKE */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
