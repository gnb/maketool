#include "filter.h"
#include <regex.h>	/* POSIX regular expression fns */

CVSID("$Id");

typedef struct
{
    char *instate;
    regex_t regexp;
    char *file_str;
    char *line_str;
    char *col_str;
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
    f->instate = strdup(instate);
    f->code = code;
    f->file_str = strdup(file_str);
    f->line_str = strdup(line_str);
    f->col_str = strdup(col_str);
    f->comment = strdup(comment);
    
    /* TODO: this is O(N^2) - try prepending then reversing O(N) */
    filters = g_list_append(filters, f);
    return f;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_load(void)
{
    filter_add(
    	"",
	"^[^:]+:[0-9]+: \\(Each undeclared identifier is reported only once",
	FR_INFORMATION,
	"",
	"",
	"",
    	"gcc spurious message #1");

    filter_add(
    	"",
	"^[^:]+:[0-9]+: for each function it appears in.\\)",
	FR_INFORMATION,
	"",
	"",
	"",
    	"gcc spurious message #2");
	
    filter_add(
    	"",
	"^([^:]+):([0-9]+): warning:",
	FR_WARNING,
	"\\1",
	"\\2",
	"",
    	"gcc warnings");
	
    filter_add(
    	"",
	"^([^:]+):([0-9]+): ",
	FR_ERROR,
	"\\1",
	"\\2",
	"",
    	"gcc errors");

#ifdef __hpux
    filter_add(
    	"",				/* state */
	"(CC|cpp): \"([^\"]*)\", line ([0-9]+): error", /* regexp */
	FR_ERROR,			/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
    	"HP-UX old CC/cpp error");	/* comment */
	
    filter_add(
    	"",				/* state */
	"(CC|cpp): \"([^\"]*)\", line ([0-9]+): warning", /* regexp */
	FR_WARNING,			/* code */
	"\\2",				/* file */
	"\\3",				/* line */
	"",				/* col */
    	"HP-UX old CC/cpp warning");	/* comment */
#endif

#ifdef __sun
    /*TODO: Solaris compilers*/
#endif

    filter_add(
    	"",				/* state */
	"(Semantic|Syntax|Parser) error at line ([0-9]+), column ([0-9]+), file[ \t]*([^ \t]*):", /* regexp */
	FR_ERROR,			/* code */
	"\\4",				/* file */
	"\\2",				/* line */
	"\\3",				/* col */
    	"Oracle Pro/C error");		/* comment */
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
    char *out,
    const char *line,
    regmatch_t *matches)
{
    for ( ; *in ; in++)
    {
    	if (*in == '\\' && in[1] >= '1' && in[1] <= '9')
	{
	    int n = (in[1] - '0');
	    if (matches[n].rm_so >= 0)
	    {
	    	int len = matches[n].rm_eo - matches[n].rm_so;
		memcpy(out, line + matches[n].rm_so, len);
		out += len;
	    }
	    in++;
	    /*TODO: check result_len */
	}
	else
	    *out++ = *in;
    }
    *out = '\0';
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define NUM_MATCHES 9

void
filter_apply_one(
    Filter *f,
    const char *line,
    FilterResult *result)
{
    regmatch_t matches[NUM_MATCHES];
    static char buf[1024];
    
    if (regexec(&f->regexp, line, NUM_MATCHES, matches, 0))
    {
    	result->code = FR_UNDEFINED;	/* no match */
	return;
    }

    /*
     * Set the result using the matched subexpressions.
     */
    filter_replace_matches(f->line_str, buf, line, matches);
    result->line = atoi(buf);
    filter_replace_matches(f->col_str, buf, line, matches);
    result->column = atoi(buf);
    filter_replace_matches(f->file_str, buf, line, matches);
    result->file = buf;
    result->code = f->code;
}

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
	fprintf(stderr, "filter [%s] on \"%s\" -> %d\n",
		f->comment, line, (int)result->code);
#endif
	if (result->code != FR_UNDEFINED)
	    return;
    }
    result->code = FR_UNDEFINED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
