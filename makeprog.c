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

CVSID("$Id: makeprog.c,v 1.4 2003-05-24 05:48:21 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
mp_which_makefile(const MakeProgram *mp)
{
    const char * const *fn;
    
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
#if HAVE_SUN_MAKE
extern const MakeProgram makeprog_sun_make;
#endif

const MakeProgram * const makeprograms[] = 
{
&makeprog_gmake,
&makeprog_pmake,
#if HAVE_IRIX_SMAKE
&makeprog_irix_smake,
#endif
#if HAVE_SUN_MAKE
&makeprog_sun_make,
#endif
0
};

const MakeProgram *
mp_find(const char *name)
{
    int i;
    
    for (i = 0 ; makeprograms[i] != 0 ; i++)
    {
    	if (!strcmp(makeprograms[i]->name, name))
	    return makeprograms[i];
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
