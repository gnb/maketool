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

CVSID("$Id: filter.c,v 1.49 2003-10-26 09:05:14 gnb Exp $");

typedef struct
{
    char *name;
    GList *filters; 	/* list of Filter's */
} FilterSet;

typedef struct
{
    int nmatches;
    int ntries;
} FilterStats;

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
    FilterStats stats;
} Filter;

static GList *filters;
static GList *filters_by_lead[256]; 	/* organised by leading character */
static GList *filter_sets;  /* in order encountered */
static FilterSet *curr_filter_set;
const char *filter_state = "";
extern const char *argv0;
static FilterStats total_stats;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Calculate and display the set of possible characters
 * with which lines matching this regexp could start.
 * Assumes the regexp is well formed, does little syntax
 * checking.
 */
static void
filter_calc_leads(const char *regexp, unsigned char suitable[256])
{
    const char *r = regexp;
    int i;
#if DEBUG > 5
#define suits(i, b, c) \
    { \
    	int _i = (i); \
	int _b = (b); \
	fprintf(stderr, "filter_calc_leads: suitable[%d] = %d (%s)\n", _i, _b, (c)); \
	suitable[_i] = _b; \
    }
#define suitall(b, c) \
    { \
	int _b = (b); \
	fprintf(stderr, "filter_calc_leads: suitable[*] = %d (%s)\n", _b, (c)); \
    	memset(suitable, _b, 256); \
    }
#define dmsg1(fmt, a1) \
    fprintf(stderr, "filter_calc_leads: "fmt"\n", (a1))
#else
#define suits(i, b, c) \
    	suitable[(i)] = (b);
#define suitall(b, c) \
    	memset(suitable, (b), 256);
#define dmsg1(fmt, a1)
#endif

    dmsg1("%s", "");
    dmsg1("starting regexp \"%s\"", regexp);
    dmsg1("%s", "");

    if (*r != '^')
    {
	suitall(TRUE, "not anchored");
	return;
    }
    r++;	/* skip leading '^' */
    suitall(FALSE, "initial values");

    if (*r == '\\')
    {
    	suits((int)r[1], TRUE, "escaped char must be a literal");
    }    
    else if (*r == '$' && r[1] == '\0')
    {
    	suits(0, TRUE, "only an empty string matches");
    }
    else
    {
    	for (;;)
	{
    	    gboolean ingroup = FALSE;
	    int nbranches = 0;
	    gboolean negbracket = FALSE;
	    gboolean nullbranch = FALSE;

	    if (*r == '(')
	    {
		dmsg1("starting group \"%s\"", r);
		ingroup = TRUE;
    		r++;   /* skip grouping operator */
	    }

    	    for (;;)
	    {    	
		if (*r == '[')
		{
		    /* handle bracket expressions [xyz] [^xyz] [x-z] [^x-z] */
    		    const char *brstart;
		    gboolean b = TRUE;

		    dmsg1("bracket expression \"%s\"", r);
		    r++;
		    if (*r == '^')
		    {
			suitall(TRUE, "negative bracket expression");
			b = FALSE;
			r++;
			negbracket = TRUE;
		    }
    		    brstart = r;
    		    for ( ; *r && *r != ']' ; r++)
		    {
			if (*r == '-' && r != brstart && r[1] != ']')
			{
	    		    for (i = r[-1] ; i <= r[1] ; i++)
				suits(i, b, "bracket expression range");
			}
			else
			    suits((int)*r, b, "bracket expression atom");
		    }
		}
		else if (ingroup && *r == '|')
		{
		    dmsg1("null branch \"%s\"", r);
		    nullbranch = TRUE;
		}
		else if (*r == '.')
		{
		    suitall(TRUE, "dot");
    	    	}
		else if (!ingroup && *r == '$')
		{
		    dmsg1("anchored at end \"%s\"", r);
		    break;
    	    	}
		else
		{
    		    suits((int)*r, TRUE, "literal");
    	    	}

		nbranches++;
		if (ingroup)
		{
	    	    /* advance to next branch or end */
		    for ( ; *r && *r != '|' && *r != ')' ; r++)
			;
		    if (*r == '|')
		    {
			r++;
			dmsg1("next branch \"%s\"", r);
			continue;
		    }
		    dmsg1("group ends before \"%s\"", r+1);
		}
		r++;
		break;
	    }
	    /*
	     * This code doesn't handle the case ^(foo|[^f]) because a negative
	     * bracket expression in a branch incorrectly overwrites all the other
	     * branches' results.  Happily we don't have any such expressions to
	     * deal with.  But just in case, assert it didn't happen.
	     */
	    assert(!ingroup || nbranches == 1 || !negbracket);
	    
	    /*
	     * Previous expression can occur zero times, so the first
	     * character might match the next expression.  Keep going.
	     * Example: "^[ \t]*(|[^ \t:#]+/)[-/a-z0-9]+..."
	     *            ^^^^^ ^^
	     * The nullbranch case works in GNU libc but is not portable
	     * (in particular FreeBSD regcomp() is known to consider it
	     * an error), so assert that it hasn't happened.
	     */
	    assert(!nullbranch);
	    if (*r == '*')
	    {
		r++;
		dmsg1("expression may be null, continuing with \"%s\"", r);
		continue;
	    }
	    break;
    	}
    }
    
#if DEBUG
    fprintf(stderr, "filter_calc_leads: [%s] -> \n\t\t\"", regexp);
    for (i = 0 ; i < 256 ; i++)
    {
    	if (suitable[i])
	{
	    if (isprint(i))
	    	fputc(i, stderr);
	    else
	    	fprintf(stderr, "\\%03o", i);
	}
    }
    fprintf(stderr, "\"\n");
#endif
}

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
    int i;
    unsigned char leads[256];
    
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
    memset(&f->stats, 0, sizeof(f->stats));

    if ((f->set = curr_filter_set) != 0)
    	f->set->filters = g_list_append(f->set->filters, f);
    
    filters = g_list_append(filters, f);

    filter_calc_leads(regexp, leads);
    for (i = 0 ; i < 256 ; i++)
    {
    	if (leads[i])
	    filters_by_lead[i] = g_list_append(filters_by_lead[i], f);
    }

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
    /*
     * The filters in the "Generic UNIX C compiler" and "GNU cross-compilers"
     * sets don't detect errors, they just provide summaries of compile lines.
     * They are interspersed for historical reasons, it's not yet clear whether
     * they need to be.
     *
     * Sadly, <> don't do quite what I expected in regexps, so they've been
     * replaced with the horror ([ \t]|[ \t].*[ \t]) which matches a string of
     * 1 or more chars starting and ending with whitespace.
     *
     * The order in which filters appear here controls the order in which they
     * are evaluated when parsing logs.  Two factors control the order:
     * 1.  More commonly matched filters should be earlier, to reduce the
     *     number of regexec()s needed to parse logs.
     * 2.  Potentially a line can match two or more filters, so the "right"
     *     one needs to be earlier.
     */
    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(/[^ \t:#]+/)*(cc|c89|gcc|CC|c\\+\\+|g\\+\\+)([ \t]|[ \t].*[ \t])-c([ \t]|[ \t].*[ \t])([^ \t]*\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\5\\6",			/* file */
	"",				/* line */
	"",				/* col */
	"Compiling \\5\\6",		/* summary */
    	"C/C++ compile line");		/* comment */

    filter_set_start(_("GNU cross-compilers"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(/[^ \t:#]+/)*[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+).*[ \t]-c([ \t]|[ \t].*[ \t])([^ \t]+\\.)(c|C|cc|c\\+\\+|cpp)", /* regexp */
	FR_INFORMATION,			/* code */
	"\\4\\5",			/* file */
	"",				/* line */
	"",				/* col */
	"Cross-compiling \\4\\5",   	/* summary */
    	"GNU C/C++ cross-compile line"); /* comment */

    filter_set_start(_("Linux kernel build"));
    filter_add(
    	"",				/* state */
	"^(/[^ \t:#]+/)+scripts/mkdep", /* regexp */
	FR_INFORMATION,			/* code */
	"",			    	/* file */
	"",				/* line */
	"",				/* col */
	"Building dependencies",   	/* summary */
    	"Linux mkdep line"); 	    	/* comment */

    filter_set_start(_("GNU make directory messages"));

    filter_add(
    	"",				/* state */
	"^(gmake|make|smake|pmake)\\[([0-9]+)\\]: Entering directory `([^']+)'", /* regexp */
	FR_PUSHDIR,			/* code */
	"\\3",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"",		    	    	/* summary */
    	"make recursion - push");	/* comment */

    filter_add(
    	"",				/* state */
	"^(gmake|make|smake|pmake)\\[([0-9]+)\\]: Leaving directory `([^']+)'", /* regexp */
	FR_POPDIR,			/* code */
	"\\3",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"",				/* summary */
    	"make recursion - pop");	/* comment */

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
	"^\\(\"([^\"]+)\", line ([0-9]+)\\) [Ee]rror: (.*)$",	/* regexp */
	FR_ERROR,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"bison errors");		/* comment */

    /* this regexp is triggered by some MIPSpro messages as well as flex */
    filter_add(
    	"",				/* state */
	"^\"([^\"]*)\", line ([0-9]+): [Ww]arning: (.*)$",	/* regexp */
	FR_WARNING,			/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",				/* summary */
    	"flex warnings");	    	/* comment */

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
	"^cfe: Error: ([^ \t,]+):[ \t]*([0-9]+): (.*)$", /* regexp */
	FR_ERROR,	    	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cpp error");	/* comment */

    filter_add(
    	"",			    	/* state */
	"^cfe: Warning [0-9]+: ([^ \t,]+):[ \t]*([0-9]+): (.*)$", /* regexp */
	FR_WARNING,	    	    	/* code */
	"\\1",				/* file */
	"\\2",				/* line */
	"",				/* col */
	"\\3",	    	    	    	/* summary */
    	"new MIPSpro cpp warning");	/* comment */

    filter_add(
    	"",			    	/* state */
	"^(as|as1): Warning: ([^ \t,]+), line ([0-9]+): (.*)$", /* regexp */
	FR_WARNING,	    	    	/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
	"\\4",	    	    	    	/* summary */
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
	
    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(/[^ \t:#]+/)*(cc|c89|gcc|CC|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Linking \\3",			/* summary */
    	"C/C++ link line");		/* comment */

    filter_set_start(_("GNU cross-compilers"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(/[^ \t:#]+/)*[-/a-z0-9]+-(cc|gcc|c\\+\\+|g\\+\\+|ld).*[ \t]+-o[ \t]+([^ \t]+)", /* regexp */
	FR_INFORMATION,			/* code */
	"",				/* file */
	"",				/* line */
	"",				/* col */
	"Cross-linking \\3",		/* summary */
    	"GNU C/C++ cross-link line");	/* comment */

    filter_set_start(_("Generic UNIX C compiler"));
    filter_add(
    	"",				/* state */
	"^[ \t]*(/[^ \t:#]+/)*ar[ \t][ \t]*[rc][a-z]*[ \t][ \t]*(lib[^ \t]*.a)", /* regexp */
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

    f->stats.ntries++;
    total_stats.ntries++;

    if (regexec(&f->regexp, line, NUM_MATCHES, matches, 0))
	return FALSE;

    f->stats.nmatches++;
    total_stats.nmatches++;

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
		    char *sum = g_strconcat(result->summary, " ", buf.data, (char *)0);
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

#if DEBUG
static const char *
code_as_string(FilterCode code)
{
    static const char * const names[] = 
    {
	"UNDEFINED",
	"INFORMATION",
	"WARNING",
	"ERROR",
	"BUILDSTART",
	"CHANGEDIR",
	"PUSHDIR",
	"POPDIR",
	"DONE"
    };
    static char buf[128];
    
    buf[0] = '\0';
    if (code & FR_PENDING)
    {
    	code &= ~FR_PENDING;
	strcpy(buf, "PENDING|");
    }
    if (code >= 0 && code <= FR_DONE)
    	strcat(buf, names[code]);
    else
    	sprintf(buf+strlen(buf), "%d (unknown)", code);
	
    return buf;
}
#endif

void
filter_apply(const char *line, FilterResult *result)
{
    GList *fl;
    gboolean matched;
    
    fl = filters_by_lead[(int)line[0]];

    for ( ; fl != 0 ; fl=fl->next)
    {
	Filter *f = (Filter*)fl->data;

	matched = filter_apply_one(f, line, result);
#if DEBUG > 2
	fprintf(stderr, "filter [%s] on \"%s\" -> %s %d (%s) state=\"%s\"\n",
		f->comment,
		line, 
		(matched ? "MATCH" : ""),
		(int)result->code,
		safe_str(result->summary),
		filter_state);
#endif
	if (matched)
	{
#if DEBUG
    	    switch ((result->code & FR_CODE_MASK))
	    {
	    case FR_ERROR:
    	    case FR_WARNING:
	    	fprintf(stderr, "filter [%s] matched \"%s\" as %s\n",
		    	    f->comment, line, code_as_string(result->code));
		break;
	    default:
	    }
#endif
	    return;
	}
    }
    result->code = FR_UNDEFINED;
    filter_state = "";
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG
static void
filter_post_one(const char *name, FilterStats *fs)
{
    fprintf(stderr, "[%s] %d / %d = %g%%\n",
    	name,
	fs->nmatches,
    	fs->ntries,
	(100.0 * (double)fs->nmatches / (double)fs->ntries));
    memset(fs, 0, sizeof(*fs));
}
#endif

void
filter_post(void)
{
#if DEBUG
    GList *iter;
        
    fprintf(stderr, "filter_post: stats\n");
    
    for (iter = filters ; iter != 0 ; iter = iter->next)
    {
    	Filter *fl = (Filter *)iter->data;

	filter_post_one(fl->comment, &fl->stats);
    }
    filter_post_one("TOTAL", &total_stats);
#endif
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
