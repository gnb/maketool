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
#include "ui.h"
#include "util.h"

CVSID("$Id: irix-makesys.c,v 1.3 2003-09-01 12:36:12 gnb Exp $");

extern const MakeProgram makeprog_irix_smake;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * MakeSystem for projects which are part of IRIX, such as the
 * IRIX kernel, libc, and applications.
 */

static gboolean
irix_probe(void)
{
    FILE *fp;
    char buf[32];
    
    if ((fp = fopen("Makefile", "r")) == 0)
    {
    	if (errno != ENOENT)
	    perror("Makefile");
    	return FALSE;
    }
    if (fgets(buf, sizeof(buf), fp) == 0)
    	buf[0] = '\0';
    fclose(fp);
    
    if (!strcmp(buf, "#!smake\n"))
    {
    	/* TODO: a better way for forcing makeprog */
    	makeprog = &makeprog_irix_smake;
    	return TRUE;
    }

    return FALSE;
}

static const char * const irix_targets[] = 
{
    "default",
    "headers",
    "exports",
    "all",
    "install",
    "clean",
    "clobber",
    0
};

const MakeSystem makesys_irix = 
{
    "irix",	    	    	/* name */
    N_("IRIX makefile system"), /* label */
    irix_probe,     	    	/* probe */
    0,	    	    	    	/* parent */
    TRUE,  	    	    	/* automatic */
    "Makefile",     	    	/* makefile */
    0,  	    	    	/* makefile_deps */
    irix_targets,   	    	/* standard_targets */
    0	    	    	    	/* commands */
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
