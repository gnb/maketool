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

#ifndef _SPAWN_H_
#define _SPAWN_H_

#include "common.h"
#include "glib_extra.h"

typedef void (*SpawnInputFunc)(int len, const char *buf, gpointer);

pid_t
spawn_simple(
    const char *command,
    GUnixReapFunc reaper,	/* may be 0 */
    gpointer reaper_data);

pid_t
spawn_with_output(
    const char *command,
    GUnixReapFunc reaper,	/* may be 0 */
    SpawnInputFunc input,
    gpointer user_data,
    char **environ);		/* environment overrides may be 0 */

#endif /* _SPAWN_H_ */
