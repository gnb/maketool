#ifndef _SPAWN_H_
#define _SPAWN_H_

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <glib.h>

void
spawn_add_reap_func(
    pid_t pid,
    void (*reaper)(pid_t, int status, struct rusage *, gpointer),
    gpointer user_data);

#endif /* _SPAWN_H_ */
