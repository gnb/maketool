/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 2003 Greg Banks
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

#include "progress.h"

CVSID("$Id: progress.c,v 1.1 2003-10-03 12:15:05 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
progress_start(Progress *p, unsigned long l)
{
    p->p_where = 0;
    p->p_length = l;
    p->p_lastpc = -1;
    if (p->p_length && p->p_ops->po_start)
    	(p->p_ops->po_start)(p);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
progress_update(Progress *p, unsigned long pos)
{
    int pc;

    p->p_where = pos;
    if (p->p_length > 10000000)
    	pc = (100 * (p->p_where / p->p_length));
    else if (p->p_length > 0)
    	pc = ((100 * p->p_where) / p->p_length);
    else
    	return;

    if (pc != p->p_lastpc)
    {
	if (p->p_ops->po_changed)
    	    (p->p_ops->po_changed)(p, pc);
    	p->p_lastpc = pc;
    }
}

void
progress_delta(Progress *p, long delta)
{
    /* assumes caller doesn't push p_where below 0 or above p_length */
    progress_update(p, (unsigned long)((long)p->p_where + delta));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
progress_end(Progress *p)
{
    if (p->p_length && p->p_ops->po_end)
    	(p->p_ops->po_end)(p);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
