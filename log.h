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

#ifndef _LOG_H_
#define _LOG_H_

#include "common.h"
#include <gtk/gtk.h>
#include "filter.h"

typedef struct
{
    /* TODO: 2 verbosity levels */
    char *line;
    FilterResult res;
    GtkCTreeNode *node;
    gboolean expanded;
} LogRec;

typedef enum
{
    L_INFO, L_WARNING, L_ERROR,
    
    L_MAX
} LogSeverity;

typedef enum
{
    LF_SHOW_INFO		=1<<0,
    LF_SHOW_WARNINGS		=1<<1,
    LF_SHOW_ERRORS		=1<<2
} LogFlags;

void logInit(GtkWidget *);

void logClear(void);
void logCollapseAll(void);
gboolean logIsEmpty(void);
void logStartBuild(const char *message);
void logEndBuild(const char *target);

LogRec *logSelected(void);
void logSetSelected(LogRec *);
LogRec *logNextError(LogRec *);
LogRec *logPrevError(LogRec *);

int logGetFlags(void);
void logSetFlags(int);
int logNumErrors(void);
int logNumWarnings(void);

void logSave(const char *file);
void logOpen(const char *file);
LogRec *logAddLine(const char *line);

#endif /* _LOG_H_ */
