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
