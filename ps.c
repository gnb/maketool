/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999 Greg Banks
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

CVSID("$Id: ps.c,v 1.2 2000-01-04 12:01:46 gnb Exp $");

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
    int page_num;
    int num_pages;
    int line;
    int font_size;
    int lines_per_page;
    int num_styles;
    PsStyle *styles;
    PsColor default_foreground;
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
    fprintf(ps->fp, "%%!PS-Adobe-2.0\n");
    fprintf(ps->fp, "%%%%Creator: Greg Banks' Maketool %s\n", VERSION); 
    fprintf(ps->fp, "%%%%Pages: atend\n"); 
    ps->in_page = FALSE;
    ps->page_num = 1;
    ps->num_pages = 0;
    ps->line = 0;
    ps->font_size = 10;
    ps->num_styles = 0;
    ps->styles = 0;
    
    ps->default_foreground.defined = TRUE;
    ps->default_foreground.red = 0;
    ps->default_foreground.green = 0;
    ps->default_foreground.blue = 0;
    
    ps->lines_per_page = ((prefs.paper_height - prefs.margin_top - prefs.margin_bottom)
    	    	      / (ps->font_size + LINE_SPACING));

    return ps;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: landscape option? */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ps_title(PsDocument *ps, const char *title)
{
    fprintf(ps->fp, "%%%%Title: %s\n", title); 
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ps_expand_styles(PsDocument *ps, int n)
{
    if (n > ps->num_styles)
    {
    	PsStyle *old = ps->styles;
	
	ps->styles = g_new(PsStyle, n);
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

static PsColor *
ps_get_foreground(PsDocument *ps, int style)
{
    return (style < ps->num_styles &&
    	    ps->styles[style].foreground.defined ?
	    &ps->styles[style].foreground : 0);
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

static PsColor *
ps_get_background(PsDocument *ps, int style)
{
    return (style < ps->num_styles &&
    	    ps->styles[style].background.defined ?
	    &ps->styles[style].background : 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char prologue[] = "\
%%EndComments\n\
/MaketoolDict 100 dict def\n\
MaketoolDict begin\n\
/Helvetica-Roman findfont 10 scalefont /_f exch def\n\
/bd{bind def}bind def\n\
/m{moveto}bd\n\
/l{lineto}bd\n\
/c{setrgbcolor}bd\n\
/b{4 dict begin\n\
 /h exch def /w exch def /y exch def /x exch def\n\
 x y moveto x w add y lineto x w add y h add lineto x y h add lineto\n\
 closepath\n\
 end}bd\n\
/s{stroke}bd\n\
/f{fill}bd\n\
/t{show}bd\n\
/bp{_f setfont 0 0 0 setrgbcolor 1 setlinewidth}bd\n\
/ep{showpage}bd\n\
end\n\
%%EndProlog\n\
";

void
ps_prologue(PsDocument *ps)
{
    fputs(prologue, ps->fp);
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
	fputs(page_trailer, ps->fp);
    ps->in_page = FALSE;
}

static void
ps_begin_page(PsDocument *ps, int n)
{
    fprintf(ps->fp, "%%Page %d %d\n", n, n);
    fputs(page_setup, ps->fp);
    
    /* Draw a box around the usable area */
    fprintf(ps->fp, "%d %d %d %d b s\n",
    	prefs.margin_left,
	prefs.margin_top,
	prefs.paper_width - prefs.margin_left - prefs.margin_right,
	prefs.paper_height - prefs.margin_top - prefs.margin_bottom);

    ps->page_num = n;
    ps->num_pages++;
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

static void
ps_color(PsDocument *ps, const PsColor *col)
{
    if (col != 0)
	fprintf(ps->fp, "%.3g %.3g %.3g c\n",
    	    col->red, col->green, col->blue);
}

void
ps_line(
    PsDocument *ps,
    const char *text,
    int style,
    int indent_level)
{
    double x, y;
    PsColor *f, *b;
    
    f = ps_get_foreground(ps, style);
    b = ps_get_background(ps, style);
    
    if (ps->line == 0)
    	ps_next_page(ps);

    y = prefs.paper_height - prefs.margin_bottom -
    	    	    (ps->line+1) * (ps->font_size + LINE_SPACING);
    x = prefs.margin_left + indent_level * INDENT;

    if (b != 0)
    {
    	/* set to background colour */
	ps_color(ps, b);
    	/* box around line */
	fprintf(ps->fp, "%d %d %d %d b f\n",
    	    prefs.margin_left,
	    (int)y-LINE_SPACING/2-(int)(0.3 * ps->font_size),  /* hack for descenders */
	    prefs.paper_width - prefs.margin_left - prefs.margin_right,
	    ps->font_size + LINE_SPACING);
	/* set the colour back */
    }
    ps_color(ps, (f == 0 ? &ps->default_foreground : f));
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
%%%%Trailer\n\
%%%%Pages: %d\n\
%%%%EOF\n\
";

void    
ps_end(PsDocument *ps)
{
    ps_end_page(ps);
    fprintf(ps->fp, trailer, ps->num_pages);
    fflush(ps->fp);
    
    g_free(ps);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
