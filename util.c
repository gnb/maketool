#include "util.h"

CVSID("$Id: util.c,v 1.7 1999-05-25 12:11:36 gnb Exp $");

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
   	(e)->data = ((e)->data == 0 ? malloc((e)->available) : realloc((e)->data, (e)->available)); \
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
estring_truncate(estring *e)
{
    e->length = 0;
    e->data[0] = '\0';
}

void
estring_free(estring *e)
{
   if (e->data != 0)
   {
   	free(e->data);
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
		    char *sub = malloc(sublen+1);
		    memcpy(sub, in+5, sublen);
		    sub[sublen] = '\0';
	    	    do_expands(sub, out, expands);
		    free(sub);
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
/*END*/
