#ifndef _SPAWN_H_
#define _SPAWN_H_

#include "common.h"
#include <sys/resource.h>
#include "glib_extra.h"

typedef void (*SpawnInputFunc)(int len, const char *buf, gpointer);

pid_t
spawn_simple(
    const char *command,
    GUnixReapFunc reaper,	/* may be 0 */
    gpointer reaper_data);

pid_t
spawn_with_output(
    const char *command,
    GUnixReapFunc reaper,	/* may be 0 */
    SpawnInputFunc input,
    gpointer user_data,
    char **environ);		/* environment overrides may be 0 */

#endif /* _SPAWN_H_ */
