/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2000 Greg Banks
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
#include <sys/stat.h>

CVSID("$Id: util.c,v 1.13 2000-07-21 07:35:32 gnb Exp $");

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
/*END*/
