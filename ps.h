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

#ifndef _PS_H_
#define _PS_H_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct _PsDocument PsDocument;

PsDocument *ps_begin(FILE *fp);
void ps_title(PsDocument *ps, const char *title);
void ps_prologue(PsDocument *ps);
void ps_num_lines(PsDocument *ps, int n);
void ps_foreground(PsDocument *ps, int style, float r, float g, float b);
void ps_background(PsDocument *ps, int style, float r, float g, float b);
void ps_line(PsDocument *ps, const char *text, int style, int indent_level);
void ps_end(PsDocument *ps);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _PS_H_ */
