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

#ifndef _common_h_
#define _common_h_

/*
 * $Id: common.h,v 1.16 2003-10-03 12:14:38 gnb Exp $
 */

#define SIGNAL_SEMANTICS_BSD	    0	/* BSD: handler automatically re-registered */
#define SIGNAL_SEMANTICS_SIGACTION  1	/* sigaction() makes it look like BSD */
#define SIGNAL_SEMANTICS_SIGVEC     2	/* sigvec() makes it look like BSD */
#define SIGNAL_SEMANTICS_SYSV	    3	/* SysV: handler needs re-registering */
#include <config.h>

#include <sys/types.h>
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <libintl.h>
#include <ctype.h>


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

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/*
 * Define as non-zero to turn on debugging prints.
 */
#ifndef DEBUG 
#define DEBUG 0
#endif

#if __GLIBC__ >= 2 && __STDC__
/* sometimes glibc is just too damn uptight */
extern FILE *popen (const char *command, const char *modes);
extern int pclose (FILE *stream);
#endif


/*
 * TODO: autoconf reckons I should check and provide
 * default definitions for WEXITSTATUS etc...yeah right.
 */

#define CVSID(s) \
	static const char * const __cvsid[2] = {(const char*)__cvsid, (s)}
#define ARRAYLEN(a) \
	(sizeof((a))/sizeof((a)[0]))
#define safe_str(s) 	((s) == 0 ? "" : (s))


#ifndef _
#define _(s)	gettext(s)
#endif
#ifndef N_
#define N_(s)	s
#endif


#endif /* _common_h_ */
