#ifndef _LOG_H_
#define _LOG_H_

#include <gtk/gtk.h>

typedef struct
{
    /* TODO: 2 verbosity levels */
    char *line;
    FilterResult res;
    GtkCTreeNode *node;
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
void logSetStyle(LogSeverity, GdkFont*, GdkColor *fore, GdkColor *back);

void logClear(void);
gboolean logIsEmpty(void);
void logStartBuild(const char *target);
void logEndBuild(const char *target);

LogRec *logSelected(void);
int logGetFlags(void);
void logSetFlags(int);
int logNumErrors(void);
int logNumWarnings(void);

void logSave(const char *file);
void logOpen(const char *file);
void logAddLine(const char *line);

#endif /* _LOG_H_ */
