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

#include "maketool.h"
#include "ps.h"
#include "util.h"
#include <time.h>

CVSID("$Id: ps.c,v 1.9 2003-08-10 10:19:39 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    gboolean defined;
    float red, green, blue;
} PsColor;

typedef struct
{
    PsColor foreground, background;
} PsStyle;

struct _PsDocument
{
    FILE *fp;
    gboolean in_page;
    char *title;
    int page_num;
    int num_pages;
    int line;
    int font_size;
    int lines_per_page;
    int num_styles;
    PsStyle *styles;
    PsColor default_foreground;
    struct
    {
    	int top, left, width, height;
    } pagebox;
};


#define LINE_SPACING 2
#define INDENT 36   	/* 1/2 in */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

PsDocument *
ps_begin(FILE *fp)
{
    PsDocument *ps;
    
    ps = g_new(PsDocument, 1);
    memset(ps, 0, sizeof(PsDocument));
    
    ps->fp = fp;
    ps->in_page = FALSE;
    ps->page_num = 0;
    ps->num_pages = 0;
    ps->line = 0;
    ps->font_size = 10;
    ps->num_styles = 0;
    ps->styles = 0;
    
    ps->default_foreground.defined = TRUE;
    ps->default_foreground.red = 0.0;
    ps->default_foreground.green = 0.0;
    ps->default_foreground.blue = 0.0;
    
    ps->lines_per_page = ((prefs.paper_height - prefs.margin_top - prefs.margin_bottom)
    	    	      / (ps->font_size + LINE_SPACING));

    ps->pagebox.left = prefs.margin_left;
    ps->pagebox.top = prefs.margin_top,
    ps->pagebox.width = prefs.paper_width - prefs.margin_left - prefs.margin_right,
    ps->pagebox.height = prefs.paper_height - prefs.margin_top - prefs.margin_bottom;

    return ps;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: landscape option? */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ps_title(PsDocument *ps, const char *title)
{
    if (ps->title != 0)
    	g_free(ps->title);
    ps->title = g_strdup(title);
}

void
ps_num_lines(PsDocument *ps, int n)
{
    ps->num_pages = (n + ps->lines_per_page - 1) / ps->lines_per_page;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ps_expand_styles(PsDocument *ps, int n)
{
    if (n > ps->num_styles)
    {
    	PsStyle *old = ps->styles;
	
	ps->styles = g_new(PsStyle, n);
	memset(ps->styles, 0, sizeof(PsStyle)*n);
	if (old != 0)
	{
	    memcpy(ps->styles, old, sizeof(PsStyle)*ps->num_styles);
	    g_free(old);
	}
	ps->num_styles = n;
    }
}

void
ps_foreground(PsDocument *ps, int style, float r, float g, float b)
{
    PsStyle *s;
    
    ps_expand_styles(ps, style+1);
    s = &ps->styles[style];
    s->foreground.defined = TRUE;
    s->foreground.red = r;
    s->foreground.green = g;
    s->foreground.blue = b;
}

void
ps_background(PsDocument *ps, int style, float r, float g, float b)
{
    PsStyle *s;
    
    ps_expand_styles(ps, style+1);
    s = &ps->styles[style];
    s->background.defined = TRUE;
    s->background.red = r;
    s->background.green = g;
    s->background.blue = b;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
ps_date_string(void)
{
    char *datebuf;
    time_t clock;

    time(&clock);
    datebuf = ctime(&clock);
    datebuf[24] = '\0';
    return g_strdup(datebuf);
}

static const char prologue[] = "\
%%EndComments\n\
%%BeginProlog\n\
/MaketoolDict 100 dict def\n\
MaketoolDict begin\n\
/Helvetica-Roman findfont 10 scalefont /_f exch def\n\
/bd{bind def}bind def\n\
/m{moveto}bd\n\
/l{lineto}bd\n\
/b{4 dict begin\n\
 /h exch def /w exch def /y exch def /x exch def\n\
 x y moveto x w add y lineto x w add y h add lineto x y h add lineto\n\
 closepath\n\
 end}bd\n\
/pb{4 copy b clip 4 copy b 0 0 0 setrgbcolor fill b 1 1 1 setrgbcolor fill}bd\n\
/pa{b 0 0 0 setrgbcolor stroke}bd\n\
/s{stroke}bd\n\
/f{fill}bd\n\
/t{show}bd\n\
/SN{/_N exch def\n\
 /_fg _N array def\n\
 /_bg _N array def\n\
 0 1 _N 1 sub{_fg exch [0 0 0] put}for\n\
 0 1 _N 1 sub{_bg exch [1 1 1] put}for\n\
}bd\n\
/SF{3 array astore _fg 3 1 roll put}bd\n\
/SB{3 array astore _bg 3 1 roll put}bd\n\
/F{_fg exch get aload pop setrgbcolor}bd\n\
/B{_bg exch get aload pop setrgbcolor}bd\n\
%multiple fills work around ghostscript 6.52 bug\n\
/bp{_f setfont 0 0 0 setrgbcolor 1 setlinewidth}bd\n\
/ep{showpage}bd\n\
end\n\
%%EndProlog\n\
";

void
ps_prologue(PsDocument *ps)
{
    int i;
    char *date;
    
    fprintf(ps->fp, "%%!PS-Adobe-2.0\n");
    fprintf(ps->fp, "%%%%Creator: Maketool %s, Copyright (c) 1999-2003 Greg Banks. All Rights Reserved.\n", VERSION); 
    assert(ps->num_pages > 0);
    fprintf(ps->fp, "%%%%Pages: %d\n", ps->num_pages); 
    fprintf(ps->fp, "%%%%Title: %s\n", ps->title);
    date = ps_date_string();
    fprintf(ps->fp, "%%%%CreationDate: %s\n", date); 
    g_free(date);
    fputs(prologue, ps->fp);
    
    
    fprintf(ps->fp, "%%%%BeginSetup\n");
    fprintf(ps->fp, "MaketoolDict begin\n");
    fprintf(ps->fp, "%d SN\n", ps->num_styles);
    for (i=0 ; i<ps->num_styles ; i++)
    {
    	if (ps->styles[i].foreground.defined)
	    fprintf(ps->fp, "%d %.3g %.3g %.3g SF\n",
	    	i,
	    	ps->styles[i].foreground.red,
	    	ps->styles[i].foreground.green,
	    	ps->styles[i].foreground.blue);
    	if (ps->styles[i].background.defined)
	    fprintf(ps->fp, "%d %.3g %.3g %.3g SB\n",
	    	i,
	    	ps->styles[i].background.red,
	    	ps->styles[i].background.green,
	    	ps->styles[i].background.blue);
    }
    fprintf(ps->fp, "end %%MaketoolDict\n");
    fprintf(ps->fp, "%%%%EndSetup\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: save & restore stuff */
static const char page_setup[] = "\
%%BeginPageSetup\n\
MaketoolDict begin\n\
bp\n\
%%EndPageSetup\n\
";

static const char page_trailer[] = "\
%%BeginPageTrailer\n\
ep\n\
end %MaketoolDict\n\
%%EndPageTrailer\n\
";

static void
ps_end_page(PsDocument *ps)
{
    if (ps->in_page)
    {
	/* Draw a box around the usable area */
	fprintf(ps->fp, "%d %d %d %d pa\n",
    	    ps->pagebox.left, ps->pagebox.top,
	    ps->pagebox.width, ps->pagebox.height);
	fputs(page_trailer, ps->fp);
    }
    ps->in_page = FALSE;
}

static void
ps_begin_page(PsDocument *ps, int n)
{
    fprintf(ps->fp, "%%%%Page: %d %d\n", n, n);
    fputs(page_setup, ps->fp);
    
    /* TODO: draw page/numpages outside usable area */

    /* Clip to the usable area */
    fprintf(ps->fp, "%d %d %d %d pb\n",
    	ps->pagebox.left, ps->pagebox.top,
	ps->pagebox.width, ps->pagebox.height);
    
    ps->page_num = n;
    ps->in_page = TRUE;
}

static void
ps_next_page(PsDocument *ps)
{
    /* End the previous page if any */
    ps_end_page(ps);
    ps_begin_page(ps, ps->page_num+1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ps_put_escaped_string(PsDocument *ps, const char *text)
{
    static const char escapable[] = "\n\r\t\b\f\\()";
    
    for ( ; *text ; text++)
    {
    	if (strchr(escapable, *text) != 0)
	    fprintf(ps->fp, "\\%03o", (int)*text);
	else
	    fputc(*text, ps->fp);
    }
}

void
ps_line(
    PsDocument *ps,
    const char *text,
    int style,
    int indent_level)
{
    double x, y;
    
    if (style < 0 || style >= ps->num_styles)
    	style = 0;  	/* default style */
	
    if (ps->line == 0)
    	ps_next_page(ps);

    y = prefs.paper_height - prefs.margin_bottom -
    	    	    (ps->line+1) * (ps->font_size + LINE_SPACING);
    x = prefs.margin_left + indent_level * INDENT;

    if (ps->styles[style].background.defined)
    {
	/* set to background colour */
	fprintf(ps->fp, "%d B\n", style);
	/* box around line */
	fprintf(ps->fp, "%d %d %d %d b f\n",
    	    prefs.margin_left,
	    (int)y-LINE_SPACING/2-(int)(0.3 * ps->font_size),  /* hack for descenders */
	    prefs.paper_width - prefs.margin_left - prefs.margin_right,
	    ps->font_size + LINE_SPACING);
    }
    
    /* set to foreground colour */
    fprintf(ps->fp, "%d F\n", style);
    fprintf(ps->fp, "%g %g m\n", x, y);
    fprintf(ps->fp, "(");
    ps_put_escaped_string(ps, text);
    fprintf(ps->fp, ") t\n");

    if (++ps->line == ps->lines_per_page)
    	ps->line = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: save& restore stuff */
static const char trailer[] = "\
%%Trailer\n\
%%EOF\n\
";

void    
ps_end(PsDocument *ps)
{
    ps_end_page(ps);
    fputs(trailer, ps->fp);
    fflush(ps->fp);

    if (ps->title != 0)
    	g_free(ps->title);    
    g_free(ps);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
