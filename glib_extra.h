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

#ifndef _GLIB_EXTRA_H_
#define _GLIB_EXTRA_H_

#include "common.h"
#include <sys/resource.h>

typedef void (*GUnixReapFunc)(pid_t, int status, struct rusage *, gpointer);

void
g_unix_reap_init(void);
void
g_unix_add_reap_func(
    pid_t pid,
    GUnixReapFunc reaper,	/* may be 0 -- uses default internal func */
    gpointer user_data);

#endif /* _GLIB_EXTRA_H_ */
