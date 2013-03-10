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

#include "util.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

CVSID("$Id: util.c,v 1.21 2004-11-07 05:35:19 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
estring_init(estring *e)
{
    e->data = 0;
    e->length = 0;
    e->available = 0;
}

#define _estring_expand_by(e, dl) \
   if ((e)->length + (dl) + 1 > (e)->available) \
   { \
   	(e)->available += MIN(1024, ((e)->length + (dl) + 1 - (e)->available)); \
   	(e)->data = ((e)->data == 0 ? g_new(char, (e)->available) : g_renew(char, (e)->data, (e)->available)); \
   }

void
estring_append_string(estring *e, const char *str)
{
    if (str != 0 && *str != '\0')
	estring_append_chars(e, str, strlen(str));
}

void
estring_append_char(estring *e, char c)
{
    _estring_expand_by(e, 1);
    e->data[e->length++] = c;
    e->data[e->length] = '\0';
}

void
estring_append_chars(estring *e, const char *buf, int len)
{
    _estring_expand_by(e, len);
    memcpy(&e->data[e->length], buf, len);
    e->length += len;
    e->data[e->length] = '\0';
}

void
estring_append_printf(estring *e, const char *fmt, ...)
{
    va_list args;
    int len;

    /* ensure enough space exists for result, possibly too much */    
    va_start(args, fmt);
    len = g_printf_string_upper_bound(fmt, args);
    va_end(args);
    _estring_expand_by(e, len);

    /* format the string into the new space */    
    va_start(args, fmt);
    vsprintf(e->data+e->length, fmt, args);
    va_end(args);
    e->length += strlen(e->data+e->length);
}

void
estring_truncate(estring *e)
{
    e->length = 0;
    if (e->data != 0)
	e->data[0] = '\0';
}

/* remove leading whitespace */
void
estring_chug(estring *e)
{
    int skip;

    for (skip = 0 ;
    	 skip < e->length && isspace(e->data[skip]) ;
	 skip++)
	;
	
    if (skip != 0)
    {
	e->length -= skip;
    	memmove(e->data, e->data+skip, e->length);
	e->data[e->length] = '\0';
    }
}

/* remove trailing whitespace */
void
estring_chomp(estring *e)
{
    int len;

    for (len = e->length ;
    	 len > 0 && isspace(e->data[len-1]) ;
	 len--)
	;
	
    if (len != e->length)
    {
	e->length = len;
	e->data[len] = '\0';
    }
}

void
estring_free(estring *e)
{
   if (e->data != 0)
   {
   	g_free(e->data);
	e->data = 0;
	e->length = 0;
	e->available = 0;
   }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char _util_metachars[] = "'`\"&|$(){};<>\\#*?";


int
strprefix(const char *str, const char *pref)
{
    int sl = strlen(str), pl = strlen(pref);
    if (sl < pl)
    	return -1;
    return strncmp(str, pref, pl);
}


char *
strescape(const char *s)
{
    estring e;
    
    estring_init(&e);
    
    for ( ; *s ; s++)
    {
    	if (ismetachar(*s) || isspace(*s))
	    estring_append_char(&e, '\\');
	estring_append_char(&e, *s);
    }
    
    return (e.length == 0 ? g_strdup("") : e.data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define LEFT_CURLY '{'
#define RIGHT_CURLY '}'

static void
do_expands(const char *in, estring *out, const char *expands[256])
{
    while (*in)
    {
    	if (*in == '%')
	{
	    if (in[1] == LEFT_CURLY)
	    {	
	    	char *x;
		int len;
		gboolean doit = FALSE;
		
		if ((x = strchr(in, RIGHT_CURLY)) == 0)
		    return;
		len = (x - in) + 1;

		switch (in[4])
		{
		case '+':
		    doit = (expands[(int)in[2]] != 0);
		    break;
		}
		if (doit)
		{
		    int sublen = len-6;
		    char *sub = g_new(char, sublen+1);
		    memcpy(sub, in+5, sublen);
		    sub[sublen] = '\0';
	    	    do_expands(sub, out, expands);
		    g_free(sub);
		}
		in += len;
	    }
	    else
	    {
		estring_append_string(out, expands[(int)in[1]]);
		in += 2;
	    }
	}
	else
	    estring_append_char(out, *in++);
    }
}

char *
expand_string(const char *in, const char *expands[256])
{
    estring out;
    
    estring_init(&out);
    do_expands(in, &out, expands);
        
#if DEBUG
    fprintf(stderr, "expand_string(): \"%s\" -> \"%s\"\n", in, out.data);
#endif
    return out.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
file_exists(const char *pathname)
{
    struct stat sb;
    
    return (stat(pathname, &sb) >= 0 && S_ISREG(sb.st_mode));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
file_is_directory(const char *pathname)
{
    struct stat sb;
    
    return (stat(pathname, &sb) >= 0 && S_ISDIR(sb.st_mode));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
file_home(void)
{
    static char *home = 0;

    if (home == 0)
    {
    	home = getenv("HOME");
	if (home == 0)
	    home = "";
	home = g_strdup(home);
    }

    assert(home != 0);
    return home;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *currdir = 0;

const char *
file_current(void)
{
    if (currdir == 0)
    	currdir = g_get_current_dir();
    return currdir;
}

int
file_change_current(const char *dir)
{
    if (chdir(dir) < 0)
    	return -1;

    if (currdir != 0)
    {
    	g_free(currdir);
	currdir = 0;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: handle path components "." and ".." */
char *
file_normalise(const char *path)
{
    if (path[0] == '/')
    	return g_strdup(path);
    
    return g_strconcat(file_current(), "/", path, (char *)0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * If `pref' is the initial subset of pathname components of `path',
 * return a new string, else return 0.  The returned string has
 * the prefix replaced with `replace1', or `replace0' if the 
 * prefix is the whole path.
 */
static char *
file_denormalise_1(
    const char *path,
    const char *pref,
    const char *replace0,
    const char *replace1)
{
    int preflen = strlen(pref);

    if (!strncmp(path, pref, preflen) &&
    	(path[preflen] == '\0' || path[preflen] == '/'))
    {
    	path += preflen;
	while (*path == '/')
    	    path++;
	if (*path == '\0')
	    return g_strdup(replace0);
	return g_strconcat(replace1, path, (char *)0);
    }
    return 0;
}

char *
file_denormalise(const char *path, unsigned flags)
{
    char *d;
    
    if ((flags & DEN_PWD) &&
    	(d = file_denormalise_1(path, file_current(), ".", "")) != 0)
    	return d;
    if ((flags & DEN_HOME) &&
    	(d = file_denormalise_1(path, file_home(), "~", "~/")) != 0)
    	return d;
    return g_strdup(path);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
time_mark(time_mark_t *mark)
{
    gettimeofday(mark, 0);
}

static void
time_elapsed(time_mark_t *mark, struct timeval *delta)
{
    struct timeval now;
    
    gettimeofday(&now, 0);
    
    delta->tv_sec = now.tv_sec - mark->tv_sec;
    if (now.tv_usec > mark->tv_usec)
    {
    	delta->tv_usec = now.tv_usec - mark->tv_usec;
    }
    else
    {
    	delta->tv_usec = 1000000 - now.tv_usec + mark->tv_usec;
	delta->tv_sec--;
    }
}

double
time_elapsed_seconds(time_mark_t *mark)
{
    struct timeval delta;
    
    time_elapsed(mark, &delta);
    return (double)delta.tv_sec + (double)delta.tv_usec / 1.0e6;
}

char *
time_elapsed_str(time_mark_t *mark)
{
    struct timeval delta;
    estring e;
    long sec;
    int i;
    static const char *labels[] = {
    	N_("day"),
	N_("hour"),
	N_("min"),
	N_("sec")
    };
    static long factors[] = { 86400, 3600, 60, 0 };
    
    time_elapsed(mark, &delta);
    estring_init(&e);
    
    sec = delta.tv_sec;
    for (i = 0 ; factors[i] ; i++)
    {
	long t = sec / factors[i];
	sec %= factors[i];

    	if (t)
	{
    	    if (e.length)
		estring_append_char(&e, ' ');
    	    estring_append_printf(&e, "%ld %s", t, _(labels[i]));
    	}
    }
    
    if (sec || !e.length)
    {
    	if (e.length)
	    estring_append_char(&e, ' ');
    	estring_append_printf(&e, "%ld.%01ld %s",
	    sec, (delta.tv_usec + 50000) / 100000, _(labels[i]));
    }
    
    assert(e.length > 0);
    return e.data;
}

char *
time_start_str(time_mark_t *mark)
{
    char buf[256];
    struct tm tm;
    
    strftime(buf, sizeof(buf),
    	     "%Y/%m/%d %H:%M:%S",
    	     localtime_r(&mark->tv_sec, &tm));
    return g_strdup(buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
