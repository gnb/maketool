#ifndef _GLIB_EXTRA_H_
#define _GLIB_EXTRA_H_

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <glib.h>

typedef void (*GUnixReapFunc)(pid_t, int status, struct rusage *, gpointer);

void
g_unix_add_reap_func(
    pid_t pid,
    GUnixReapFunc reaper,	/* may be 0 -- uses default internal func */
    gpointer user_data);

#endif /* _GLIB_EXTRA_H_ */
