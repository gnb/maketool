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

#ifndef _FILTER_H_
#define _FILTER_H_

#include "common.h"

typedef enum 
{
    FR_UNDEFINED,	/* this filter can't tell */
    
    FR_INFORMATION,	/* filter has classified as one of these severities */
    FR_WARNING,		/* result_str is "file|line|column" */
    FR_ERROR,
    FR_BUILDSTART,
    
    FR_CHANGEDIR,	/* result_str is directory to change to */
    FR_PUSHDIR,		/* result_str is directory to push on dir stack */
    FR_POPDIR,		/* pop directory stack */
    
    FR_DONE 	    	/* end multiple-line match without modifying result */

    /* extra information bitfields */
#define FR_PENDING  	(1<<8)	/* start multiple-line match */
#define FR_CODE_MASK	0xff
} FilterCode;

typedef struct
{
    FilterCode code;
    char *file;
    int line;
    int column;
    char *summary;
} FilterResult;

void filter_load(void);
void filter_init(void);
void filter_apply(const char *line, FilterResult *result);

#endif /* _FILTER_H_ */
