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
#include "maketool_task.h"
#include "util.h"
#if HAVE_REGCOMP
#include <regex.h>	/* POSIX regular expression fns */
#else
#error Why are you even bothering to compile with POSIX regular expressions?
#endif

CVSID("$Id: gmake.c,v 1.2 2003-05-04 04:12:57 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_makefile_flags(estring *e)
{
    if (prefs.makefile != 0)
    {
    	estring_append_string(e, "-f ");
	estring_append_string(e, prefs.makefile);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_parallel_flags(estring *e)
{
    switch (prefs.run_how)
    {
    case RUN_SERIES:
    	break;	/* no flag */
    case RUN_PARALLEL_PROC:
    	if (prefs.run_processes > 0)
    	    estring_append_printf(e, "-j%d", prefs.run_processes);
	else
    	    estring_append_string(e, "-j");
    	break;
    case RUN_PARALLEL_LOAD:
    	if (prefs.run_load > 0)
    	    estring_append_printf(e, "-l%.2g", (gfloat)prefs.run_load / 10.0);
	else
    	    estring_append_string(e, "-l");
    	break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_keep_going_flags(estring *e)
{
    if (prefs.keep_going)
    	estring_append_string(e, "-k");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_dryrun_flags(estring *e)
{
    if (prefs.dryrun)
	estring_append_string(e, "-n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_version_flags(estring *e)
{
    estring_append_string(e, "--version");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
gmake_list_targets_flags(estring *e)
{
    estring_append_string(e, "-n -p");
}

typedef struct
{
    Task task;
    GPtrArray *targets;
#define NUM_ERROR_LINES 16
    int nlines;
    estring error_buf;	    /* first NUM_ERROR_LINES of output */
    gboolean is_target;
    gboolean is_file;
    gboolean no_rule_message_seen;
    regex_t target_re;
} GmakeListTargetsTask;

static void
gmake_list_targets_reap(Task *task)
{
    GmakeListTargetsTask *ltt = (GmakeListTargetsTask *)task;
     
    if (!task_is_successful(task) &&
    	(!ltt->no_rule_message_seen || ltt->targets->len == 0))
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
gmake_list_targets_input(Task *task, int len, const char *buf)
{
    GmakeListTargetsTask *ltt = (GmakeListTargetsTask *)task;
    regmatch_t match;

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
    if (is_const_prefix(buf, "# Files"))
    {
    	ltt->is_file = TRUE;
	return;
    }
    if (is_const_prefix(buf, "# Not a target:"))
    {
    	ltt->is_target = FALSE;
	return;
    }
    if (is_const_prefix(buf, "# VPATH Search Paths"))
    {
    	ltt->is_file = FALSE;
	return;
    }
    if (strstr(buf, "*** No rule to make target") != 0)
    {
    	ltt->no_rule_message_seen = TRUE;
    	return;
    }
    
    /* Match line against the target regular expression. */
    if (regexec(&ltt->target_re, buf, /*nmatches*/1, &match, 0) == 0)
    {
    	if (ltt->is_file && ltt->is_target)
	{
	    /* extract target name from line...steal the buffer temporarily */
	    char *targ, oldc;
	    int end;

	    targ = (char *)buf+match.rm_so;
	    end = match.rm_eo - match.rm_so - 1;
	    
	    oldc = targ[end];
	    targ[end] = '\0';

	    if (filter_target(targ))
		g_ptr_array_add(ltt->targets, g_strdup(targ));

	    targ[end] = oldc;
    	}
	ltt->is_target = TRUE;
    }
}
#undef is_const_prefix


static void
gmake_list_targets_destroy(Task *task)
{
    GmakeListTargetsTask *ltt = (GmakeListTargetsTask *)task;

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

static TaskOps gmake_list_targets_ops =
{
0,  	    	    	    	/* start */
gmake_list_targets_input,	/* input */
gmake_list_targets_reap, 	/* reap */
gmake_list_targets_destroy	/* destroy */
};


static Task *
gmake_list_targets_task(char *cmd)
{
    GmakeListTargetsTask *ltt = g_new(GmakeListTargetsTask, 1);
    int err;

    ltt->is_target = TRUE;
    ltt->is_file = FALSE;
    ltt->no_rule_message_seen = FALSE;
    ltt->targets = g_ptr_array_new();
    err = regcomp(&ltt->target_re, "^([^#: \t]+)[ \t]*:", REG_EXTENDED);
    assert(err == 0);
    ltt->nlines = 0;
    estring_init(&ltt->error_buf);
    
    return task_create(
    	(Task *)ltt,
	cmd,
	prefs.var_environment,
	&gmake_list_targets_ops,
	TASK_LINEMODE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char * const gmake_makefiles[] = 
{
    "GNUmakefile",
    "makefile",
    "Makefile",
    0
};

#include "babygnu_l.xpm"

MakeProgram makeprog_gmake = 
{
    "gmake",
    GMAKE,
    N_("GNU make"),
    babygnu_l_xpm,
    gmake_makefiles,
    gmake_makefile_flags,
    gmake_parallel_flags,
    gmake_keep_going_flags,
    gmake_dryrun_flags,
    gmake_version_flags,
    gmake_list_targets_flags,
    gmake_list_targets_task,
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
