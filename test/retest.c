#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <regex.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
typedef int 	bool_t;
#define boolstr(b)  	((b) ? "true" : "false")


typedef struct _context_s 
{
    struct _context_s *next;
    FILE *infp;
    FILE *outfp;
    char *filename;
    unsigned long lineno;
    bool_t prompt;
    bool_t echo;
#ifdef HAVE_READLINE
    bool_t readline;
#endif
    char *arg_buf;
} context_t;

typedef struct
{
    regex_t regexp;
    bool_t compiled;
    bool_t matched;
#define NUM_MATCHES 10
    char *matches[NUM_MATCHES];
} recontext_t;

typedef struct
{
    const char *name;
    bool_t (*func)(context_t *);
    const char *usage;
    const char *help;
} command_t;

static context_t *constack;
static FILE *recordfp;
static recontext_t re;
const char *argv0;
const char *prompt;

#define safestr(s)  	((s) == 0 ? "" : (s))

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
strndup(const char *s, unsigned int len)
{
    char *x;
    
    if ((x = malloc(len+1)) == 0)
    	return 0;
    memcpy(x, s, len);
    x[len] = '\0';

    return x;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static context_t *
con_push(const char *filename, FILE *infp)
{
    context_t *con;
    bool_t echo = FALSE;
    
    if (infp == 0)
    {
    	if ((infp = fopen(filename, "r")) == 0)
	{
	    perror(filename);
	    return 0;
    	}
    	echo = TRUE;
    }

    if ((con = malloc(sizeof(context_t))) == 0)
    	return con;
    memset(con, 0, sizeof(*con));
    con->filename = strdup(filename);
    con->infp = infp;
    con->echo = echo;
    if (constack)
    {
    	con->outfp = constack->outfp;
    }
    else
    {
    	con->outfp = stdout;
    }

    con->next = constack;
    constack = con;
    
    return con;
}

static void
con_pop(void)
{
    context_t *con = constack;
    
    assert(con != 0);
    constack = constack->next;
    
    if (constack == 0 || constack->infp != con->infp)
    	fclose(con->infp);
    if ((constack == 0 || constack->outfp != con->outfp) && con->outfp != stdout)
    	fclose(con->outfp);

    free(con->filename);
    free(con);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void con_error(context_t *con, const char *fmt, ...)
#ifdef __GNUC__
__attribute__ (( format(printf,2,3) ))
#endif
;
 
void
con_error(context_t *con, const char *fmt, ...)
{
    va_list args;
    
    fflush(stdout);
    va_start(args, fmt);
    fprintf(stderr, "%s:%lu:", con->filename, con->lineno);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    fflush(stderr);
}


void con_fatal(context_t *con, const char *fmt, ...)
#ifdef __GNUC__
__attribute__ (( noreturn ))
__attribute__ (( format(printf,2,3) ))
#endif
;

void
con_fatal(context_t *con, const char *fmt, ...)
{
    va_list args;
    
    fflush(stdout);
    va_start(args, fmt);
    fprintf(stderr, "%s:%lu:FATAL:", con->filename, con->lineno);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    fflush(stderr);

    exit(1);
}

#define con_syntax_error(con) \
    con_error((con), "syntax error")

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *
get_next_arg(context_t *con)
{
    char *x = strtok(con->arg_buf, " \t\r\n");
    con->arg_buf = 0;
    return x;
}

static const char *
get_all_args(context_t *con)
{
    return con->arg_buf;
}
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
clear_matches(void)
{
    int i;

    for (i = 0 ; i < NUM_MATCHES ; i++)
    	if (re.matches[i] != 0)
	    free(re.matches[i]);
    memset(re.matches, 0, sizeof(re.matches));
}

static bool_t
command_comp(context_t *con)
{
    const char *pattern = get_all_args(con);
    int err;

    if (re.compiled)
	regfree(&re.regexp);
    clear_matches();

    if ((err = regcomp(&re.regexp, pattern, REG_EXTENDED)) != 0)
    {
    	char buf[1024];
    	regerror(err, &re.regexp, buf, sizeof(buf));
	con_error(con, "error in regexp \"%s\": %s", pattern, buf);
    	re.compiled = FALSE;
    }
    else
    	re.compiled = TRUE;
    return TRUE;
}

static bool_t
command_exec(context_t *con)
{
    const char *string = get_all_args(con);
    regmatch_t matches[NUM_MATCHES];

    fprintf(con->outfp, "\"%s\" -> \n    ", string);
    memset(matches, 0, sizeof(matches));
    if (!regexec(&re.regexp, string, NUM_MATCHES, matches, 0))
    {
    	int i;
	
	re.matched = TRUE;
	fprintf(con->outfp, "matched\n");
	memset(re.matches, 0, sizeof(re.matches));
	for (i = 0 ; i < NUM_MATCHES ; i++)
	{
	    if (matches[i].rm_so < 0)
	    	continue;
    	    re.matches[i] = strndup(string + matches[i].rm_so,
	    	    	    	    matches[i].rm_eo - matches[i].rm_so);
	    fprintf(con->outfp, "    \\%d=[%d,%d]\"%s\"\n",
	    	    i,
		    matches[i].rm_so,
		    matches[i].rm_eo,
		    re.matches[i]);
	}
    }
    else
    {
	re.matched = FALSE;
	fprintf(con->outfp, "unmatched\n");
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static bool_t
command_record(context_t *con)
{
    const char *filename = get_next_arg(con);

    if (filename == 0)
    {
    	con_syntax_error(con);
	return TRUE;
    }
    if ((recordfp == fopen(filename, "w")) == 0)
    {
    	perror(filename);
	return TRUE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static bool_t
command_include(context_t *con)
{
    const char *filename = get_next_arg(con);
    context_t *newcon;

    if (filename == 0)
    {
    	con_syntax_error(con);
	return TRUE;
    }
    newcon = con_push(filename, 0);
    if (newcon == 0 && !con->prompt)
    	exit(1);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static bool_t
command_expect(context_t *con)
{
    const char *predicate = get_next_arg(con);
    bool_t sense = TRUE;
    bool_t predval;
    
    if (predicate == 0)
    {
    	con_syntax_error(con);
    	return TRUE;
    }
    
    if (*predicate == '!')
    {
    	sense = !sense;
	predicate++;
    }
    
    if (!strcmp(predicate, "compiled"))
    {
    	predval = re.compiled;
    }
    else if (!strcmp(predicate, "matched"))
    {
    	predval = re.matched;
    }
    else if (!strcmp(predicate, "match"))
    {
    	const char *nstr, *val;
	int n;
	
	if ((nstr = get_next_arg(con)) == 0 ||
	    (n = atoi(nstr)) < 0 ||
	    n > NUM_MATCHES ||
	    (val = get_next_arg(con)) == 0)
	{
    	    con_syntax_error(con);
    	    return TRUE;
	}
	if (!strcmp(val, "\"\""))
	    val = "";
    	predval = !strcmp(safestr(re.matches[n]), safestr(val));
    }
    else
    {
    	con_error(con, "unknown predicate \"%s\"", predicate);
    	return TRUE;
    }
    
    if (predval != sense)
    	con_fatal(con, "FAILED expected predicate %s=%s got %s",
	    predicate, boolstr(sense), boolstr(predval));
    
    return TRUE;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static bool_t
command_quit(context_t *con)
{
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const command_t commands[];

static bool_t
command_help(context_t *con)
{
    const char *arg1 = get_next_arg(con);
    const command_t *c;
    char usage[128];
    
    for (c = commands ; c->name ; c++)
    {
    	if (arg1 != 0 && strcmp(arg1, c->name))
	    continue;
    	snprintf(usage, sizeof(usage), "%s %s", c->name, c->usage);
	fprintf(con->outfp, "%-20s %s\n", usage, c->help);
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const command_t commands[] = 
{
{"comp",    command_comp,   "PATTERN",	    "compile a regular expression"},
{"exec",    command_exec,   "STRING",	    "compare STRING to the compiled regexp"},
{"expect",  command_expect, "[!]PREDICATE", "assert test results"},
{"record",  command_record, "FILENAME",     "begin recording commands to FILENAME"},
{"include", command_include,"FILENAME",     "execute retest commands in FILENAME"},
{"quit",    command_quit,   "",     	    "exit retest"},
{"help",    command_help,   "",     	    "print this message"},
{0, 	    0,	    	    0}
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
get_line(context_t *con)
{
#ifdef HAVE_READLINE
    if (con->readline)
    {
    	return readline(prompt);
    }
    else
#endif
    {
	char *p, buf[1024];

	if (con->prompt)
	{
	    fputs(prompt, con->outfp);
	    fflush(con->outfp);
	}
	if (fgets(buf, sizeof(buf), con->infp) == 0)
	    return 0;
	p = buf+strlen(buf)-1;
	while (p >= buf && (*p == '\n' || *p == '\r'))
	    *p-- = '\0';
	return strdup(buf);
    }
}

static bool_t
do_command(context_t *con)
{
    char *command;
    char *args;
    bool_t ret = TRUE;
    const command_t *c;
    char *line = 0;

    for (;;)
    {
    	if (line != 0)
	    free(line);
    	line = get_line(con);
	if (line == 0)
	{
	    con_pop();
	    return TRUE;
	}
	con->lineno++;

	if (line[0] == '#')
	{
	    /* comment */
	    if (con->echo)
	    	fprintf(con->outfp, "%s:%lu:%s\n",
		    	con->filename, con->lineno, line);
    	    if (recordfp)
	    	fprintf(recordfp, "%s\n", line);
	    continue;
	}
	
	if ((command = strtok(line, " \t\r\n")) == 0)
	{
	    /* empty or whitespace only line */
	    if (con->echo)
	    	fprintf(con->outfp, "%s:%lu:\n",
		    	con->filename, con->lineno);
    	    if (recordfp)
	    	fprintf(recordfp, "\n");
	    continue;
	}

	args = strtok(0, "\n\r");
	
#ifdef HAVE_READLINE
	if (con->readline)
    	{
	    char *hist = malloc(strlen(command)+strlen(args)+2);
	    strcpy(hist, command);
	    strcat(hist, " ");
	    strcat(hist, args);
	    add_history(hist);
	    free(hist);
	}
#endif

	for (c = commands ; c->name ; c++)
	{
	    if (!strcmp(command, c->name))
	    	break;
	}
	if (c->name == 0)
	{
	    con_error(con, "unknown command \"%s\"", command);
	}
	else
	{
	    const char *safeargs = (args == 0 ? "" : args);
	    if (con->echo)
	    	fprintf(con->outfp, "%s:%lu:%s %s\n",
		    	con->filename, con->lineno, command, safeargs);
    	    if (recordfp)
	    	fprintf(recordfp, "%s %s\n", command, safeargs);

    	    con->arg_buf = args;
	    ret = (*c->func)(con);
	}
	free(line);
	break;	
    }
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
do_commands(void)
{
    while (constack != 0 && do_command(constack))
    	;
    while (constack != 0)
    	con_pop();
}

static void
do_command_file(const char *filename)
{
    context_t *con;

    if ((con = con_push(filename, 0)) == 0)
    	exit(1);    
    do_commands();
}

static void
do_commands_interactive(void)
{
    context_t *con;

    con = con_push("<stdin>", stdin);
    con->prompt = isatty(fileno(stdin));
#ifdef HAVE_READLINE
    con->readline = con->prompt;
    using_history();
#endif
    do_commands();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *
basename_c(const char *path)
{
    const char *t = strrchr(path, '/');
    return (t == 0 ? path : ++t);
}

int
main(int argc, char **argv)
{
    int i;
    
    argv0 = basename_c(argv[0]);
    {
    	char buf[128];
	snprintf(buf, sizeof(buf), "%s> ", argv0);
	prompt = strdup(buf);
    }

    if (argc == 1)
    	do_commands_interactive();
    else
        for (i = 1 ; i < argc ; i++)
	    do_command_file(argv[i]);

    if (recordfp)
    	fclose(recordfp);

    return 0;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
