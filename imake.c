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

CVSID("$Id: imake.c,v 1.2 2003-05-24 05:48:20 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * MakeSystem for projects using the old X11 Imake system.
 */

static gboolean
imake_probe(void)
{
    return (file_exists("Imakefile"));
}

static const char * const imake_deps[] = 
{
    "Imakefile",
    0
};

static const MakeCommand imake_commands[] = 
{
    {N_("Run _xmkmf"), "xmkmf"},
    {0}
};

const MakeSystem makesys_imake = 
{
    "imake",	    	    	/* name */
    N_("XConsortium Imake"),	/* label */
    imake_probe,    	    	/* probe */
    0,	    	    	    	/* parent */
    TRUE,  	    	    	/* automatic */
    "Makefile",     	    	/* makefile */
    imake_deps,	    	    	/* makefile_deps */
    0,	    	    	    	/* standard_targets */
    imake_commands  	    	/* commands */
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
