#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <regex.h>	/* POSIX regular expression fns */
#include "filter.h"

typedef struct
{
    char *instate;
    regex_t regexp;
    char *result_str;
    FilterResult result;
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
    const char *result_str,
    FilterResult result,
    char *comment)
{
    Filter *f = g_new(Filter, 1);
    guint err;
    
    if ((err = regcomp(&f->regexp, regexp, REG_EXTENDED)) != 0)
    {
        char errbuf[1024];
    	regerror(err, &f->regexp, errbuf, sizeof(errbuf));
	/* TODO: decent error reporting mechanism */
	fprintf(stderr, "%s: error in regular expression \"%s\": %s\n",
		argv0, regexp, errbuf);
	regfree(&f->regexp);
	g_free(f);
	return 0;
    }
    f->instate = strdup(instate);
    f->result_str = strdup(result_str);
    f->result = result;
    f->comment = strdup(comment);
    
    /* TODO: this is O(N^2) - try prepending then reversing O(N) */
    filters = g_list_append(filters, f);
    return f;
}    

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_load(void)
{
    filter_add("", "^[^:]+:[0-9]+: \\(Each undeclared identifier is reported only once", "", FR_INFORMATION,
    	"gcc spurious message #1");
    filter_add("", "^[^:]+:[0-9]+: for each function it appears in.\\)", "", FR_INFORMATION,
    	"gcc spurious message #2");
    filter_add("", "^([^:]+):([0-9]+): warning:", "\\1|\\2|-1", FR_WARNING,
    	"gcc warnings");
    filter_add("", "^([^:]+):([0-9]+): ", "\\1|\\2|-1", FR_ERROR,
    	"gcc errors");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
filter_init(void)
{
    filterState = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define NUM_MATCHES 9

FilterResult
filter_apply_one(
    Filter *f,
    const char *line,
    char *result_str,
    int result_len)
{
    const char *in;
    char *out;
    regmatch_t matches[NUM_MATCHES];
    
    if (regexec(&f->regexp, line, NUM_MATCHES, matches, 0))
    	return FR_UNDEFINED;	/* no match */

    /*
     * Set the result_str using the matched subexpressions.
     */	
    for (in = f->result_str, out = result_str ; *in ; in++)
    {
    	if (*in == '\\' && *in >= '1' && *in <= '9')
	{
	    int n = (*in - '1');
	    if (matches[n].rm_so >= 0)
	    {
	    	int len = matches[n].rm_eo - matches[n].rm_so + 1;
		memcpy(out, line + matches[n].rm_so, len);
		out += len;
	    }
	    /*TODO: check result_len */
	}
	else
	    *out++ = *in;
    }
    *out = '\0';
    
    /* TODO:??? parse into file,line,col?? */
    
    return f->result;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

FilterResult
filter_apply(const char *line, char *result_str, int result_len)
{
    GList *fl;
    
    for (fl=filters ; fl != 0 ; fl=fl->next)
    {
    	FilterResult res;
	Filter *f = (Filter*)fl->data;
	res = filter_apply_one(f, line, result_str, result_len);
#if DEBUG
	fprintf(stderr, "filter [%s] on \"%s\" -> %d\n", f->comment, line, (int)res);
#endif
	if (res != FR_UNDEFINED)
	    return res;
    }
    return FR_UNDEFINED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
