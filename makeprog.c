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

CVSID("$Id: makeprog.c,v 1.1 2003-02-09 05:01:58 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
mp_which_makefile(const MakeProgram *mp)
{
    const char **fn;
    
    for (fn = mp->makefiles ; *fn ; fn++)
    {
    	if (file_exists(*fn))
	    return *fn;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern const MakeProgram makeprog_gmake;
extern const MakeProgram makeprog_pmake;
#if HAVE_IRIX_SMAKE
extern const MakeProgram makeprog_irix_smake;
#endif

const MakeProgram * const makeprograms[] = 
{
&makeprog_gmake,
&makeprog_pmake,
#if HAVE_IRIX_SMAKE
&makeprog_irix,
#endif
0
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
