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

#include "maketool.h"
#include "util.h"
#if HAVE_REGCOMP
#include <regex.h>	/* POSIX regular expression fns */
#else
#error Why are you even bothering to compile with POSIX regular expressions?
#endif

CVSID("$Id: pmake.c,v 1.2 2003-05-04 04:21:49 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
pmake_makefile_flags(estring *e)
{
    if (prefs.makefile != 0)
    {
    	estring_append_string(e, "-f ");
	estring_append_string(e, prefs.makefile);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
pmake_parallel_flags(estring *e)
{
    if (prefs.run_how == RUN_PARALLEL_PROC && prefs.run_processes > 0)
    	estring_append_printf(e, "-J%d", prefs.run_processes);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
pmake_keep_going_flags(estring *e)
{
    if (prefs.keep_going)
    	estring_append_string(e, "-k");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
pmake_dryrun_flags(estring *e)
{
    if (prefs.dryrun)
	estring_append_string(e, "-n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* 
 * pmake is impossibly modest...there is no way to emit even
 * a version string, let alone a copyright/authorship message.
 * So we do the best we can.
 */
static const char version_hack[] = 
"-n _no_such_target_ > /dev/null 2>&1 ;"
"echo \"BSD Pmake\" ;"
"echo \"by the NetBSD Project\" ;"
"echo \"http://www.netbsd.org/\" ;"
;

static void
pmake_version_flags(estring *e)
{
    estring_append_string(e, version_hack);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
pmake_list_targets_flags(estring *e)
{
    estring_append_string(e, "-q -d g1 _no_such_target_");
}

#if HAVE_IRIX_SMAKE
static void
irix_smake_list_targets_flags(estring *e)
{
    estring_append_string(e, "-q -p 1 _no_such_target_");
}
#endif

typedef struct
{
    Task task;
    GPtrArray *targets;
#define NUM_ERROR_LINES 16
    int nlines;
    estring error_buf;	    /* first NUM_ERROR_LINES of output */
    gboolean is_target;
    gboolean is_file;
    regex_t target_re;
} PmakeListTargetsTask;

static void
pmake_list_targets_reap(Task *task)
{
    PmakeListTargetsTask *ltt = (PmakeListTargetsTask *)task;
     
    /*
     * On my Linux box, pmake gets confused, emits spurious
     * errors and exits with a failure code despite having
     * correctly generated the target list.  So don't report
     * an error unless no targets were parsed.
     */
    if (!task_is_successful(task) && ltt->targets->len == 0)
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
#define is_const_prefix(s, p)	    (!strncmp(buf, p, sizeof(p)-1))

static void
pmake_list_targets_input(Task *task, int len, const char *buf)
{
    PmakeListTargetsTask *ltt = (PmakeListTargetsTask *)task;
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

    /*
     * Drive a simple state machine which figures out which
     * part of the "gmake -p" output is target listings.
     */
    if (is_const_prefix(buf, "#*** Input graph:"))
    {
    	ltt->is_file = TRUE;
	return;
    }
    if (is_const_prefix(buf, "#*** Global Variables:"))
    {
    	ltt->is_file = FALSE;
	return;
    }
    
    /* Match line against the target regular expression. */
    if (regexec(&ltt->target_re, buf, /*nmatches*/2, match, 0) == 0)
    {
    	if (ltt->is_file && ltt->is_target)
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
	ltt->is_target = TRUE;
    }
}
#undef is_const_prefix


static void
pmake_list_targets_destroy(Task *task)
{
    PmakeListTargetsTask *ltt = (PmakeListTargetsTask *)task;

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

static TaskOps pmake_list_targets_ops =
{
0,  	    	    	    	/* start */
pmake_list_targets_input,	/* input */
pmake_list_targets_reap, 	/* reap */
pmake_list_targets_destroy	/* destroy */
};


static Task *
pmake_list_targets_task(char *cmd)
{
    PmakeListTargetsTask *ltt = g_new(PmakeListTargetsTask, 1);
    int err;

    ltt->is_target = TRUE;
    ltt->is_file = FALSE;
    ltt->targets = g_ptr_array_new();
    err = regcomp(&ltt->target_re, "^([^#:! \t]+)[ \t]*[:!]", REG_EXTENDED);
    assert(err == 0);
    ltt->nlines = 0;
    estring_init(&ltt->error_buf);
    
    return task_create(
    	(Task *)ltt,
	cmd,
	prefs.var_environment,
	&pmake_list_targets_ops,
	TASK_LINEMODE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char * const pmake_makefiles[] = 
{
    "Makefile",
    "makefile",
    0
};

#include "BSD-daemon.xpm"

MakeProgram makeprog_pmake = 
{
    "pmake",
    "pmake",
    N_("BSD 4.4 pmake"),
    BSD_daemon_xpm,
    pmake_makefiles,
    pmake_makefile_flags,
    pmake_parallel_flags,
    pmake_keep_going_flags,
    pmake_dryrun_flags,
    pmake_version_flags,
    pmake_list_targets_flags,
    pmake_list_targets_task
};

#if HAVE_IRIX_SMAKE
MakeProgram makeprog_irix_smake = 
{
    "irix-smake",
    "smake",
    N_("IRIX smake"),
    /*logo_xpm*/0,
    pmake_makefiles,
    pmake_makefile_flags,
    pmake_parallel_flags,
    pmake_keep_going_flags,
    pmake_dryrun_flags,
    pmake_version_flags,
    irix_smake_list_targets_flags,
    pmake_list_targets_task
};
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
