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

CVSID("$Id: ps.c,v 1.1 1999-12-20 11:04:40 gnb Exp $");

static gboolean in_page;
static int page_num;
static int num_pages;
static int line;
static int font_size = 10;
static int lines_per_page;

#define LINE_SPACING 2
#define INDENT 36   	/* 1/2 in */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ps_begin_file(FILE *fp)
{
    fprintf(fp, "%%!PS-Adobe-2.0\n");
    fprintf(fp, "%%%%Creator: Greg Banks' Maketool %s\n", VERSION); 
    fprintf(fp, "%%%%Pages: atend\n"); 
    in_page = FALSE;
    page_num = 1;
    num_pages = 0;
    line = 0;
    
    lines_per_page = ((prefs.paper_height - prefs.margin_top - prefs.margin_bottom)
    	    	      / (font_size + LINE_SPACING));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: landscape option? */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ps_title(FILE *fp, const char *title)
{
    fprintf(fp, "%%%%Title: %s\n", title); 
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
/b{4 dict begin\n\
/h exch def /w exch def /y exch def /x exch def\n\
x y moveto x w add y lineto x w add y h add lineto x y h add lineto\n\
closepath\n\
end}bd\n\
/s{stroke}bd\n\
/t{show}bd\n\
/bp{_f setfont 0 0 0 setrgbcolor 1 setlinewidth}bd\n\
/ep{showpage}bd\n\
end\n\
%%EndProlog\n\
";

void
ps_prologue(FILE *fp)
{
    fputs(prologue, fp);
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
ps_end_page(FILE *fp)
{
    if (in_page)
	fputs(page_trailer, fp);
    in_page = FALSE;
}

static void
ps_begin_page(FILE *fp, int n)
{
    fprintf(fp, "%%Page %d %d\n", n, n);
    fputs(page_setup, fp);
    
    /* Draw a box around the usable area */
    fprintf(fp, "%d %d %d %d b s\n",
    	prefs.margin_left,
	prefs.margin_top,
	prefs.paper_width - prefs.margin_left - prefs.margin_right,
	prefs.paper_height - prefs.margin_top - prefs.margin_bottom);

    page_num = n;
    num_pages++;
    in_page = TRUE;

}

static void
ps_next_page(FILE *fp)
{
    /* End the previous page if any */
    ps_end_page(fp);
    ps_begin_page(fp, page_num+1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ps_put_escaped_string(FILE *fp, const char *text)
{
    static const char escapable[] = "\n\r\t\b\f\\()";
    
    for ( ; *text ; text++)
    {
    	if (strchr(escapable, *text) != 0)
	    fprintf(fp, "\\%03o", (int)*text);
	else
	    fputc(*text, fp);
    }
}

void
ps_line(FILE *fp, const char *text, int indent_level)
{
    double x, y;
    
    if (line == 0)
    	ps_next_page(fp);

    y = prefs.paper_height - prefs.margin_bottom -
    	    	    (line+1) * (font_size + LINE_SPACING);
    x = prefs.margin_left + indent_level * INDENT;

    fprintf(fp, "%g %g m\n", x, y);
    fprintf(fp, "(");
    ps_put_escaped_string(fp, text);
    fprintf(fp, ") t\n");

    if (++line == lines_per_page)
    	line = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: save& restore stuff */
static const char trailer[] = "\
%%%%Trailer\n\
%%%%Pages: %d\n\
%%%%EOF\n\
";

void    
ps_end_file(FILE *fp)
{
    ps_end_page(fp);
    fprintf(fp, trailer, num_pages);
    fflush(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
