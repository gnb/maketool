/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999 Greg Banks
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

#ifndef _common_h_
#define _common_h_

/*
 * $Id: common.h,v 1.4 1999-06-01 11:04:40 gnb Exp $
 */

#include <config.h>

#include <sys/types.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <libintl.h>


#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#else
#if !HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif
extern char *strchr(char *);
extern char *strrchr(char *);
#endif

#if HAVE_MEMORY_H
#include <memory.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/*
 * TODO: autoconf reckons I should check and provide
 * default definitions for WEXITSTATUS etc...yeah right.
 */

#define CVSID(s) \
	static const char * const __cvsid[2] = {(const char*)__cvsid, (s)}
#define ARRAYLEN(a) \
	(sizeof((a))/sizeof((a)[0]))


#ifndef _
#define _(s)	gettext(s)
#endif


#endif /* _common_h_ */
