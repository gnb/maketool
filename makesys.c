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
#include "util.h"

CVSID("$Id: makesys.c,v 1.4 2003-10-03 12:15:34 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Check to see if we can and should update
 * the Makefile from other files.
 */

gboolean
ms_makefile_needs_update(const MakeSystem *ms)
{
    int i;
    struct stat sb;
    time_t mf_mtime;

    if (!ms->automatic)
    	return FALSE;	    /* nothing useful to do */
	
    /*
     * Get Makefile's mod time.  If it doesn't
     * exist, we can update.
     */
    if (stat(ms->makefile, &sb) < 0)
    	return (errno == ENOENT);
    mf_mtime = sb.st_mtime;

    /*
     * Check if any of the dependencies are missing or newer.
     */
    for ( ; ms != 0 ; ms = ms->parent)
    {
    	if (ms->makefile_deps == 0)
	    continue;
	for (i = 0 ; ms->makefile_deps[i] != 0 ; i++)
	{
    	    if (stat(ms->makefile_deps[i], &sb) <= 0)
		if (errno == ENOENT)
	    	    return TRUE;
	    if (sb.st_mtime > mf_mtime)
		return TRUE;
	}
    }

    /* ok, everything seems in order, don't update */
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * These are the targets specifically mentioned in the
 * current GNU makefile standards (except `mostlyclean'
 * which is from the old standards). These targets are
 * visually separated in the Build menu.  Note that
 * each MakeSystem can override the set of standard
 * targets.
 */
static const char * const gnu_standard_targets[] = {
"all",
"install", "install-strip", "installcheck", "installdirs", "uninstall",
"mostlyclean", "clean", "distclean", "reallyclean", "maintainer-clean",
"TAGS", "tags",
"info", "dvi",
"dist",
"check",
0
};


gboolean
ms_is_standard_target(const MakeSystem *ms, const char *targ)
{
    const char * const *tp = 0;
    
    for ( ; ms != 0 ; ms = ms->parent)
    {
    	if ((tp = ms->standard_targets) != 0)
	    break;
    }
    if (tp == 0)
    	tp = gnu_standard_targets;

    for ( ; *tp ; tp++)
    {
	if (!strcmp(*tp, targ))
	    return TRUE;
    }
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
plain_probe(void)
{
    /*
     * This is the default MakeSystem after all the fancy
     * ones have failed to probe.
     */
    return TRUE;
}

static const MakeSystem makesys_plain = 
{
    "plain",	    	    	    	/* name */
    N_("plain handcrafted Makefile"),	/* label */
    plain_probe,    	    	    	/* probe */
    0,	    	    	    	    	/* parent */
    FALSE,  	    	    	    	/* automatic */
    0,	    	    	    	    	/* makefile */
    0,    	    	    	    	/* makefile_deps */
    0,    	    	    	    	/* standard_targets */
    0 	    	    	    	    	/* commands */
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern const MakeSystem makesys_am; 	    /* automake */
extern const MakeSystem makesys_ac_maint;   /* autoconf maint */
extern const MakeSystem makesys_ac_dist;    /* autoconf dist */
extern const MakeSystem makesys_imake;      /* Imake */
#if HAVE_MAKESYS_IRIX
extern const MakeSystem makesys_irix;       /* IRIX */
#endif

const MakeSystem * const makesystems[] = 
{
&makesys_am,
&makesys_ac_maint,
&makesys_ac_dist,
    /*
     * Imake is after autoconf on the assumption that Imake projects
     * get converted into autoconf projects and not vice versa, so if
     * both are present autoconf is likely to be the correct choice.
     */
&makesys_imake,
#if HAVE_MAKESYS_IRIX
&makesys_irix,
#endif
&makesys_plain,
0
};


const MakeSystem *
ms_probe(void)
{
    int i;
    
    for (i = 0 ; makesystems[i] != 0 ; i++)
    {
    	if ((*makesystems[i]->probe)())
	    return makesystems[i];
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
