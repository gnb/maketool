#ifndef _FILTER_H_
#define _FILTER_H_

typedef enum 
{
    FR_UNDEFINED,	/* this filter can't tell */
    
    FR_INFORMATION,	/* filter has classified as one of these severities */
    FR_WARNING,		/* result_str is "file|line|column" */
    FR_ERROR,
    
    FR_CHANGEDIR,	/* result_str is directory to change to */
    FR_PUSHDIR,		/* result_str is directory to push on dir stack */
    FR_POPDIR,		/* pop directory stack */
    
    FR_PENDING		/* result_str is new state for recognition machine */
} FilterCode;

typedef struct
{
    FilterCode code;
    char *file;
    int line;
    int column;
} FilterResult;

void filter_load(void);
void filter_init(void);
void filter_apply(const char *line, FilterResult *result);

#endif /* _FILTER_H_ */
