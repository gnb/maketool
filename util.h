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

#ifndef _UTIL_H_
#define _UTIL_H_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    char *data;
    int length;		/* string length not including nul */
    int available;	/* total bytes available, >= length+1 */
} estring;

void estring_init(estring *e);
void estring_append_string(estring *e, const char *str);
void estring_append_char(estring *e, char c);
void estring_append_chars(estring *e, const char *buf, int len);
void estring_append_printf(estring *e, const char *fmt, ...);
void estring_truncate(estring *e);
/* remove leading whitespace */
void estring_chug(estring *e);
/* remove trailing whitespace */
void estring_chomp(estring *e);
void estring_free(estring *e);

#define ESTRING_STATIC_INIT \
	{ 0, 0, 0 }

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* Returns non-zero iff c is a shell or make metacharacter */
extern const char _util_metachars[];
#define ismetachar(c)	(strchr(_util_metachars, (c)) != 0)


/* Returns 0 iff `pref' is the initial part of `str' */
int strprefix(const char *str, const char *pref);

/*
 * Return a new string which is the old one with
 * shell meta-characters and whitespace escaped.
 */
char *strescape(const char *s);

/* Result needs to be free()d */
char *expand_string(const char *in, const char *expands[256]);

gboolean file_exists(const char *pathname);
gboolean file_is_directory(const char *pathname);

/* Returns the user's home directory.  Never returns NULL */
const char *file_home(void);

/* Returns the cached current directory.  Never returns NULL */
const char *file_current(void);
/*
 * Change UNIX current directory and invalidate cached string.
 * Returns 0 on success or -1 and errno on failure.
 */
int file_change_current(const char *dir);

/*
 * Returns a new string with a full UNIX pathname representing
 * the `path', which may be absolute or relative.
 */
char *file_normalise(const char *path);

/*
 * Return a new string denormalised according to flags.
 * A denormalised string will have an initial prefix replaced
 * with a shorter string designed to still appear as legal
 * shell filename syntax but be easier for a human to read.
 * `flags' control which prefixes are enabled.  Always returns
 * a new string, even if it's just a copy of `path'.
 */
#define DEN_HOME    (1<<0)  	/* replace $HOME prefix with ~ */
#define DEN_PWD     (1<<1)  	/* remove current directory prefix */
#define DEN_ALL     (~0)
char *file_denormalise(const char *path, unsigned flags);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Support for timing intervals.
 */

typedef struct timeval	    time_mark_t;

/*
 * Initialise a time mark to the current time
 */
void time_mark(time_mark_t *);
/*
 * Returns the time since the mark in floating-point seconds.
 */
double time_elapsed_seconds(time_mark_t *);
/*
 * Formats an optimal human-readable string describing the
 * time since the mark.  Returns a new string which needs
 * to be g_free()d.
 */
char *time_elapsed_str(time_mark_t *);
/*
 * Formats a string describing the time mark itself.  Returns
 * a new string which needs to be g_free()d.
 */
char *time_start_str(time_mark_t *);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _UTIL_H_ */
