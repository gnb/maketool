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

#include "filter.h"
#include "util.h"
#if HAVE_REGCOMP
#include <regex.h>	/* POSIX regular expression fns */

CVSID("$Id: filter.c,v 1.25 2001-07-25 08:35:22 gnb Exp $");

typedef struct
{
    char *instate;
    regex_t regexp;
    char *file_str;
    char *line_str;
    char *col_str;
    char *summary_str;
    FilterCode code;
    char *comment;
} Filter;


static GList *filters;
const char *filterState = 0;
extern const char *argv0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static Filter *
filter_add(
    const char *instate,
    const char *regexp,
    FilterCode code,
    const char *file_str,
    const char *line_str,
    const char *col_str,
    const char *summary_str,
    const char *comment)
{
    Filter *f = g_new(Filter, 1);
    guint err;
    
    if ((err = regcomp(&f->regexp, regexp, REG_EXTENDED)) != 0)
    {
        char errbuf[1024];
    	regerror(err, &f->regexp, errbuf, sizeof(errbuf));
	/* TODO: decent error reporting mechanism */
	fprintf(stderr, _("%s: error in regular expression \"%s\": %s\n"),
		argv0, regexp, errbuf);
	regfree(&f->regexp);
	g_free(f);
	return 0;
    }
    f->instate = g_strdup(instate);
    f->code = code;
    f->file_str = g_strdup(file_str);
    f->line_str = g_strdup(line_str);
    f->col_str = g_strdup(col_str);
    f->summary_str = g_strdup(summary_str);
    f->comment = g_strdup(comment);
    
    /* TODO: this is O(N^2) - try prepending then reversing O(N) */
    filters = g_list_append(filters, f);
    return f;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_load(void)
{
    filter_add(
    	"",				/* state */
	"^[a-zA-Z0-9_-]+\\[[0-9]+\\]: Entering directory `([^']+)'", /* regexp */
	FR_PUSHDIR,			/* code */
	"\\1",				/* file */
	"",				/* line */
	"",				/* col */
	"Directory \\1",		/* summary */
    	"gmake recursion - push");	/* comment */

    filter_add(
    	"",				/* state */
	"^[a-zA-Z0-9_-]+\\[[0-9]+\\]: Leaving directory", /* regexp */
	FR_POPDIR,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,				/* summary */
    	"gmake recursion - pop");	/* comment */

    filter_add(
    	"",				/* state */
	"^[^:]+:[0-9]+: \\(Each undeclared identifier is reported only once", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",				/* summary */
    	"gcc spurious message #1");	/* comment */

    filter_add(
    	"",				/* state */
	"^[^:]+:[0-9]+: for each function it appears in.\\)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",				/* summary */
    	"gcc spurious message #2");	/* comment */
	
    filter_add(
    	"",				/* state */
	"^[^:]+: In function",	    	/* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",				/* summary */
    	"gcc spurious message #3");	/* comment */
	
    filter_add(
    	"",				/* state */
	"^([^: \t]+):([0-9]+):([0-9]+): [wW]arning:(.*)$",	/* regexp */
	FR_WARNING,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"\\3",				/* col */
	"\\4",				/* summary */
    	"new gcc warnings");		/* comment */
	
    filter_add(
    	"",				/* state */
	"^([^: \t]+):([0-9]+): [wW]arning:(.*)$",	/* regexp */
	FR_WARNING,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"gcc warnings");		/* comment */
	
    filter_add(
    	"",				/* state */
	"^([^: \t]+):([0-9]+):([0-9]+):(.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"\\3",				/* col */
	"\\4",				/* summary */
    	"new gcc errors");		/* comment */

    filter_add(
    	"",				/* state */
	"^([^: \t]+):([0-9]+):(.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"gcc errors");			/* comment */

    filter_add(
    	"",				/* state */
	"^\\(\"([^\"]+)\", line ([0-9]+)\\) error: (.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"bison errors");		/* comment */

    filter_add(
    	"",				/* state */
	"^\"([^\"]*)\", line ([0-9]+): (.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"flex errors");	    		/* comment */

    filter_add(
    	"",				/* state */
	"^In file included from ([^: \t]+):([0-9]+)[,:]$",	/* regexp */
	FR_INFORMATION,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"Included from \\1:\\2",	/* summary */
    	"gcc inclusion trace 1");	/* comment */
    filter_add(
    	"",				/* state */
	"^[ \t]+from +([^: \t]+):([0-9]+)[,:]$",	/* regexp */
	FR_INFORMATION,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"Included from \\1:\\2",	/* summary */
    	"gcc inclusion trace 2");	/* comment */

#ifdef __hpux
    filter_add(
    	"",				/* state */
	"(CC|cpp): \"([^\"]*)\", line ([0-9]+): error(.*)$", /* regexp */
	FR_ERROR,			/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	"\\4",				/* summary */
    	"HP-UX old CC/cpp error");	/* comment */
	
    filter_add(
    	"",				/* state */
	"(CC|cpp): \"([^\"]*)\", line ([0-9]+): warning(.*)$", /* regexp */
	FR_WARNING,			/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	"\\4",				/* summary */
    	"HP-UX old CC/cpp warning");	/* comment */
#endif

#ifdef __sun
    /*TODO: Solaris compilers*/
#endif

    filter_add(
    	"",				/* state */
	"(Semantic|Syntax|Parser) error at line ([0-9]+), column ([0-9]+), file[ \t]*([^ \t]*):(.*)$", /* regexp */
	FR_ERROR,			/* code */
	"\\4",				/* file */
	"\\2",				/* line */
	"\\3",				/* col */
	"\\1 error: \\5",		/* summary */
    	"Oracle Pro/C error");		/* comment */
	
    /*
     * The remaining filters don't detect errors,
     * they just provide summaries of compile lines.
     *
     * Sadly, <> don't do quite what I expected in
     * regexps, so they've been replaced with the
     * horror ([ \t]|[ \t].*[ \t]) which matches a
     * string of 1 or more chars starting and ending
     * with whitespace.
     */
    filter_add(
    	"",				/* state */
	"^(cc|c89|gcc|CC|c\\+\\+|g\\+\\+)([ \t]|[ \t].*[ \t])-c([ \t]|[ \t].*[ \t])([^ \t]*\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\4\\5",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\4\\5",		/* summary */
    	"C/C++ compile line");		/* comment */
    filter_add(
    	"",				/* state */
	"^[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+).*[ \t]-c([ \t]|[ \t].*[ \t])([^ \t]+\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\3\\4",			/* file */
	"",				/* line */
	"",				/* col */
	"Cross-compiling \\3\\4",   	/* summary */
    	"GNU C/C++ cross-compile line"); /* comment */
    filter_add(
    	"",				/* state */
	"^javac[ \t].*[ \t]([A-Za-z_*][A-Za-z0-3_*]*).java", /* regexp */
	FR_INFORMATION,			/* code */
	"\\1.java",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\1.java",		/* summary */
    	"Java compile line");		/* comment */
    filter_add(
    	"",				/* state */
	"^(cc|c89|gcc|CC|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Linking \\2",			/* summary */
    	"C/C++ link line");		/* comment */
    filter_add(
    	"",				/* state */
	"^[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Cross-linking \\2",		/* summary */
    	"GNU C/C++ cross-link line");	/* comment */
    filter_add(
    	"",				/* state */
	"^ar[ \t][ \t]*[rc][a-z]*[ \t][ \t]*(lib[^ \t]*.a)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Building library \\1",		/* summary */
    	"Archive library link line");	/* comment */
    /* TODO: support for libtool */	

#if ENABLE_MWOS
    /*
     * Support for errors generated by cross-compiling
     * for Microware OS-9 using Microware's xcc compiler.
     */
    filter_add(
    	"",				/* state */
	"^\"([^\" \t]+)\", line ([0-9]+): error:(.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"MWOS xcc errors"); 	    	/* comment */
	
    filter_add(
    	"",				/* state */
	"^\"([^\" \t]+)\", line ([0-9]+): warning:(.*)$",	/* regexp */
	FR_WARNING,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"MWOS xcc warnings");	    	/* comment */
    /*
     * Support for summarising Microware xcc compile lines.
     * Note that xcc does not use the POSIX standard compile options,
     * using e.g. -v=DIR instead of -IDIR.  In particular,
     * POSIX' -c is xcc' -r.
     */
    filter_add(
    	"",				/* state */
	"^xcc.*[ \t]-r[ \t].*[ \t]([^ \t]*\\.)(c)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\1\\2",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\1\\2",		/* summary */
    	"xcc C compile line");		/* comment */
#endif

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_init(void)
{
    filterState = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
filter_replace_matches(
    const char *in,
    estring *out,
    const char *line,
    regmatch_t *matches)
{
    estring_truncate(out);
    for ( ; *in ; in++)
    {
    	if (*in == '\\' && in[1] >= '1' && in[1] <= '9')
	{
	    int n = (in[1] - '0');
	    if (matches[n].rm_so >= 0)
	    {
	    	int len = matches[n].rm_eo - matches[n].rm_so;
		estring_append_chars(out, line + matches[n].rm_so, len);
	    }
	    in++;
	}
	else
	    estring_append_char(out, *in);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define NUM_MATCHES 9

#define safe_atoi(s)	((s) == 0 ? 0 : atoi(s))

void
filter_apply_one(
    Filter *f,
    const char *line,
    FilterResult *result)
{
    regmatch_t matches[NUM_MATCHES];
    static estring buf = ESTRING_STATIC_INIT;
    
    if (regexec(&f->regexp, line, NUM_MATCHES, matches, 0))
    {
    	result->code = FR_UNDEFINED;	/* no match */
	return;
    }

    /*
     * Set the result using the matched subexpressions.
     */
    filter_replace_matches(f->line_str, &buf, line, matches);
    result->line = safe_atoi(buf.data);
    filter_replace_matches(f->col_str, &buf, line, matches);
    result->column = safe_atoi(buf.data);
    if (f->summary_str == 0)
    {
    	result->summary = 0;
    }
    else
    {
	filter_replace_matches(f->summary_str, &buf, line, matches);
	result->summary = g_strdup(buf.data);
    }
    filter_replace_matches(f->file_str, &buf, line, matches);
    result->file = buf.data;
    result->code = f->code;
}

#undef safe_atoi

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_apply(const char *line, FilterResult *result)
{
    GList *fl;
    
    for (fl=filters ; fl != 0 ; fl=fl->next)
    {
	Filter *f = (Filter*)fl->data;
	filter_apply_one(f, line, result);
#if DEBUG
	fprintf(stderr, "filter [%s] on \"%s\" -> %d (%s)\n",
		f->comment, line, (int)result->code, safe_str(result->summary));
#endif
	if (result->code != FR_UNDEFINED)
	    return;
    }
    result->code = FR_UNDEFINED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#else /*!HAVE_REGCOMP*/
#error This version of maketool requires the POSIX regcomp function
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
