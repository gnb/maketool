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

#ifndef _progress_h_
#define _progress_h_

/*
 * $Id: progress.h,v 1.1 2003-10-03 12:15:05 gnb Exp $
 */

#include "common.h"

typedef struct _progress_s  	Progress;
typedef struct _progress_ops_s  ProgressOps;

struct _progress_ops_s
{
    void (*po_start)(Progress *p);
    void (*po_changed)(Progress *p, int pc);
    void (*po_end)(Progress *p);
};

struct _progress_s
{
    const ProgressOps *p_ops;
    unsigned long p_where;
    unsigned long p_length;
    int p_lastpc;
};

extern void progress_start(Progress *p, unsigned long l);
extern void progress_update(Progress *p, unsigned long pos);
extern void progress_delta(Progress *p, long delta);
extern void progress_end(Progress *p);

#endif /* _progress_h_ */
