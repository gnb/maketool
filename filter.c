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

#include "filter.h"
#include "util.h"
#if HAVE_REGCOMP
#include <regex.h>	/* POSIX regular expression fns */

CVSID("$Id: filter.c,v 1.37 2003-10-10 09:31:35 gnb Exp $");

typedef struct
{
    char *name;
    GList *filters; 	/* list of Filter's */
} FilterSet;

typedef struct
{
    char *instate;
    char *outstate;
    regex_t regexp;
    char *file_str;
    char *line_str;
    char *col_str;
    char *summary_str;
    FilterCode code;
    char *comment;
    FilterSet *set;
} Filter;

static GList *filters;
static GList *filter_sets;  /* in order encountered */
static FilterSet *curr_filter_set;
const char *filter_state = "";
extern const char *argv0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static Filter *
filter_add(
    const char *state,
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
    f->instate = g_strdup(state);
    if ((f->outstate = strchr(f->instate, ':')) != 0)
    {
    	*f->outstate++ = '\0';
	f->outstate = g_strdup(f->outstate);
    }
    else
    	f->outstate = g_strdup("");
    f->code = code;
    f->file_str = g_strdup(file_str);
    f->line_str = g_strdup(line_str);
    f->col_str = g_strdup(col_str);
    f->summary_str = g_strdup(summary_str);
    f->comment = g_strdup(comment);

    if ((f->set = curr_filter_set) != 0)
    	f->set->filters = g_list_append(f->set->filters, f);
    
    /* TODO: this is O(N^2) - try prepending then reversing O(N) */
    filters = g_list_append(filters, f);
    return f;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
filter_set_start(const char *name)
{
    GList *iter;
    FilterSet *fs;

    /* Try to find an existing set of the same name */    
    for (iter = filter_sets ; iter != 0 ; iter = iter->next)
    {
    	fs = (FilterSet *)iter->data;

    	if (!strcmp(name, fs->name))
	{
	    curr_filter_set = fs;
	    return;
	}
    }
    
    fs = g_new(FilterSet, 1);
    fs->name = g_strdup(name);
    fs->filters = 0;
    
    filter_sets = g_list_append(filter_sets, fs);
    curr_filter_set = fs;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_load(void)
{
    filter_set_start(_("GNU make directory messages"));

    filter_add(
    	"",				/* state */
	"^[a-zA-Z0-9_-]+\\[([0-9]+)\\]: Entering directory `([^']+)'", /* regexp */
	FR_PUSHDIR,			/* code */
	"\\2",				/* file */
	"\\1",				/* line */
	"",				/* col */
	"",		    	    	/* summary */
    	"gmake recursion - push");	/* comment */

    filter_add(
    	"",				/* state */
	"^[a-zA-Z0-9_-]+\\[([0-9]+)\\]: Leaving directory `([^']+)'", /* regexp */
	FR_POPDIR,			/* code */
	"\\2",				/* file */
	"\\1",				/* line */
	"",				/* col */
	"",				/* summary */
    	"gmake recursion - pop");	/* comment */

    filter_set_start(_("GNU Compiler Collection"));

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
	"^\\{standard input\\}:([0-9]+): [wW]arning:(.*)$",	/* regexp */
	FR_WARNING,			/* code */
	"",				/* file */
	"\\1",				/* line */
	"",				/* col */
	"\\2",				/* summary */
    	"new gas warnings");		/* comment */
	
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

    filter_set_start(_("Bison & Flex"));

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

#if ENABLE_FILTER_HPUX
    filter_set_start(_("HP-UX compilers"));

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
#endif /* ENABLE_FILTER_HPUX */

#if ENABLE_FILTER_MIPSPRO
    filter_set_start(_("IRIX MIPSpro compilers"));
    
    /*
     * IRIX smake in parallel mode emits lines like these before every
     * line of output, but because they list only the target and not the
     * source or directory they're nearly useless to us.
     */
    filter_add(
    	"",			    	/* state */
	"^--- [^ \t:]+ ---$",	    	/* regexp */
	FR_INFORMATION,	    	    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",	    	    	    	/* summary */
    	"smake target name");   	/* comment */

    /* old style MIPSpro errors and warnings */
    filter_add(
    	":mipspro1",			/* state */
	"^cc-[0-9]+ (cc|CC): ERROR File = ([^ \t,]+), Line = ([0-9]+)", /* regexp */
	FR_PENDING|FR_ERROR,	    	/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro cc/CC error");   	/* comment */
	
    filter_add(
    	":mipspro1",			/* state */
	"^cc-[0-9]+ (cc|CC): WARNING File = ([^ \t,]+), Line = ([0-9]+)", /* regexp */
	FR_PENDING|FR_WARNING,	    	/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro cc/CC warning");   	/* comment */

    filter_add(
    	":mipspro1",			/* state */
	"^cc-[0-9]+ (cc|CC): REMARK File = ([^ \t,]+), Line = ([0-9]+)", /* regexp */
	FR_PENDING|FR_WARNING,	    	/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro cc/CC remark");   	/* comment */

    filter_add(
    	"mipspro1:mipspro2",		/* state */
	"^[ \t]+(.*)$",    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"\\1",	    	    	    	/* summary */
    	"MIPSpro multiline message 1"); /* comment */

    filter_add(
    	"mipspro2:mipspro2a",		/* state */
	"^[ \t]+(.*)$",    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"\\1",	    	    	    	/* summary */
    	"MIPSpro multiline message 2"); /* comment */

    filter_add(
    	"mipspro2a:mipspro3",		/* state */
	"^$",    	    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 2"); /* comment */

    filter_add(
    	"mipspro2:mipspro3",		/* state */
	"^$",    	    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 2"); /* comment */

    filter_add(
    	"mipspro3:",		    	/* state */
	"^$",    	    	    	/* regexp */
	FR_DONE,    	    	    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 3a"); /* comment */

    filter_add(
    	"mipspro3:mipspro4",		/* state */
	"^.*$",    	    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 3"); /* comment */

    filter_add(
    	"mipspro4:mipspro5",		/* state */
	"^[ \t]+\\^$",	    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 4"); /* comment */

    filter_add(
    	"mipspro5:",		    	/* state */
	"^$",    	    	    	/* regexp */
	FR_DONE,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	0,	    	    	    	/* summary */
    	"MIPSpro multiline message 5"); /* comment */

    /* IRIX linker errors */
    filter_add(
    	"",				/* state */
	"^(ld|ld32|ld64): ERROR[ \t]+[0-9]+[ \t]*: (.*)$", /* regexp */
	FR_ERROR,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"\\2",				/* summary */
    	"MIPSpro ld error");	    	/* comment */
	
    /* new style MIPSpro errors and warnings */
    filter_add(
    	":Nmipspro1",			/* state */
	"^cfe: Error: ([^ \t,]+), line ([0-9]+): (.*)$", /* regexp */
	FR_PENDING|FR_ERROR,	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cc/CC error");   	/* comment */
	
    filter_add(
    	":Nmipspro1",			/* state */
	"^cfe: Warning [0-9]+: ([^ \t,]+), line ([0-9]+): (.*)$", /* regexp */
	FR_PENDING|FR_WARNING,	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cc/CC warning");	/* comment */

    filter_add(
    	"Nmipspro1:Nmipspro2",		/* state */
	"^ (.*)$",    	    	    	/* regexp */
	FR_PENDING|FR_UNDEFINED,    	/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",	    	    	    	/* summary */
    	"new MIPSpro multiline message 1"); /* comment */

    filter_add(
    	"Nmipspro2:",		    	/* state */
	"^ [- \t]*\\^$",    	    	/* regexp */
	FR_DONE,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"",	    	    	    	/* summary */
    	"MIPSpro multiline message 5"); /* comment */

    /* new style assembler and pre-processor errors */
    filter_add(
    	"",			    	/* state */
	"^cfe: Error: ([^ \t,]+):[ \t]*[0-9]+: (.*)$", /* regexp */
	FR_ERROR,	    	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cpp error");	/* comment */

    filter_add(
    	"",			    	/* state */
	"^cfe: Warning [0-9]+: ([^ \t,]+):[ \t]*[0-9]+: (.*)$", /* regexp */
	FR_WARNING,	    	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cpp warning");	/* comment */

    filter_add(
    	"",			    	/* state */
	"^as1: Warning: ([^ \t,]+), line [0-9]+: (.*)$", /* regexp */
	FR_WARNING,	    	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro as warning");	/* comment */
#endif /* ENABLE_FILTER_MIPSPRO */

#if ENABLE_FILTER_SUN
    filter_set_start(_("Solaris compilers (unimplemented)");
    /*TODO: Solaris compilers*/
#endif /* ENABLE_FILTER_SUN */

    filter_set_start(_("Oracle Pro/C"));

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
    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(|[^ \t:#]+/)(cc|c89|gcc|CC|c\\+\\+|g\\+\\+)([ \t]|[ \t].*[ \t])-c([ \t]|[ \t].*[ \t])([^ \t]*\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\5\\6",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\5\\6",		/* summary */
    	"C/C++ compile line");		/* comment */

    filter_set_start(_("GNU cross-compilers"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(|[^ \t:#]+/)[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+).*[ \t]-c([ \t]|[ \t].*[ \t])([^ \t]+\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\4\\5",			/* file */
	"",				/* line */
	"",				/* col */
	"Cross-compiling \\4\\5",   	/* summary */
    	"GNU C/C++ cross-compile line"); /* comment */

    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(|[^ \t:#]+/)(cc|c89|gcc|CC|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Linking \\3",			/* summary */
    	"C/C++ link line");		/* comment */

    filter_set_start(_("GNU cross-compilers"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(|[^ \t:#]+/)[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Cross-linking \\3",		/* summary */
    	"GNU C/C++ cross-link line");	/* comment */

    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(|[^ \t:#]+/)ar[ \t][ \t]*[rc][a-z]*[ \t][ \t]*(lib[^ \t]*.a)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Building library \\2",		/* summary */
    	"Archive library link line");	/* comment */
    /* TODO: support for libtool */	

    filter_set_start(_("GNU autoconf messages"));
    filter_add(
    	"",				/* state */
	"^creating ([^ \t]+)$",	    	/* regexp */
	FR_INFORMATION,			/* code */
	"\\1",				/* file */
	"",				/* line */
	"",				/* col */
	0,		    	    	/* summary */
    	"configure script output file 1"); /* comment */
    filter_add(
    	"",				/* state */
	"^([^ \t]+) is unchanged$",  	/* regexp */
	FR_INFORMATION,			/* code */
	"\\1",				/* file */
	"",				/* line */
	"",				/* col */
	0,		    	    	/* summary */
    	"configure script output file 2"); /* comment */

    filter_set_start(_("Sun Java"));
    filter_add(
   	"",				/* state */
	"^javac[ \t].*[ \t]([A-Za-z_*][A-Za-z0-3_*]*).java", /* regexp */
	FR_INFORMATION,			/* code */
	"\\1.java",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\1.java",		/* summary */
    	"Java compile line");		/* comment */


#if ENABLE_FILTER_MWOS
    /*
     * Support for errors generated by cross-compiling
     * for Microware OS-9 using Microware's xcc compiler.
     */
    filter_set_start(_("Microware OS-9 cross-compilers"));

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
#endif /* ENABLE_FILTER_MWOS */

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_init(void)
{
    filter_state = "";
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

static gboolean
filter_apply_one(
    Filter *f,
    const char *line,
    FilterResult *result)
{
    regmatch_t matches[NUM_MATCHES];
    static estring buf = ESTRING_STATIC_INIT;
    int i;
    
    if (strcmp(filter_state, f->instate))
	return FALSE;

    if (regexec(&f->regexp, line, NUM_MATCHES, matches, 0))
	return FALSE;
    filter_state = f->outstate;

    /*
     * Set (parts of) the result using the matched subexpressions.
     */
    filter_replace_matches(f->line_str, &buf, line, matches);
    if ((i = safe_atoi(buf.data)) > 0)
	result->line = i;

    filter_replace_matches(f->col_str, &buf, line, matches);
    if ((i = safe_atoi(buf.data)) > 0)
	result->column = i;

    if (f->summary_str != 0)
    {
    	if (*f->summary_str == '\0')
	{
	    result->summary = g_strdup("");
    	}
	else
	{
	    filter_replace_matches(f->summary_str, &buf, line, matches);
	    if (buf.data != 0 && *buf.data != '\0')
	    {
	    	if (result->summary != 0 &&
		    ((f->code & FR_PENDING) || f->code == FR_DONE))
		{
		    /* append to the summary */
		    char *sum = g_strconcat(result->summary, " ", buf.data, 0);
#if DEBUG
	    	    fprintf(stderr, "filter_apply_one: appending summary \"%s\" to \"%s\"\n",
			result->summary, sum);
#endif
		    g_free(result->summary);
		    result->summary = sum;
		}
		else
		{
		    /* replace or start the summary */
		    if (result->summary != 0)
		    {
#if DEBUG
	    		fprintf(stderr, "filter_apply_one: replacing summary \"%s\" with \"%s\"\n",
			    result->summary, buf.data);
#endif
			g_free(result->summary);
		    }
		    result->summary = g_strdup(buf.data);
		}
	    }
	}
    }

    filter_replace_matches(f->file_str, &buf, line, matches);
    if (buf.data != 0 && *buf.data != '\0')
	result->file = g_strdup(buf.data);

    if (f->code == FR_DONE)
    	result->code &= ~FR_PENDING;
    else if ((f->code & ~FR_PENDING) != FR_UNDEFINED)
	result->code = f->code;
    if ((f->code & FR_PENDING))
	result->code |= FR_PENDING;
	
    return TRUE;
}

#undef safe_atoi

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_apply(const char *line, FilterResult *result)
{
    GList *fl;
    gboolean matched;

    for (fl=filters ; fl != 0 ; fl=fl->next)
    {
	Filter *f = (Filter*)fl->data;
	matched = filter_apply_one(f, line, result);
#if DEBUG
	fprintf(stderr, "filter [%s] on \"%s\" -> %s %d (%s) state=\"%s\"\n",
		f->comment,
		line, 
		(matched ? "MATCH" : ""),
		(int)result->code,
		safe_str(result->summary),
		filter_state);
#endif
	if (matched)
	    return;
    }
    result->code = FR_UNDEFINED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_describe_all(estring *e, int lod, const char *indent)
{
    GList *iter1, *iter2;
    
    estring_append_string(e, "Filters:\n");

    for (iter1 = filter_sets ; iter1 != 0 ; iter1 = iter1->next)
    {
    	FilterSet *fs = (FilterSet *)iter1->data;

	estring_append_printf(e, "%s%s\n", indent, fs->name);

    	if (lod > 0)
	{
    	    for (iter2 = fs->filters ; iter2 != 0 ; iter2 = iter2->next)
	    {
		Filter *f = (Filter *)iter2->data;
		
		estring_append_printf(e, "%s%s%s\n", indent, indent, f->comment);
	    }
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#else /*!HAVE_REGCOMP*/
#error This version of maketool requires the POSIX regcomp function
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
