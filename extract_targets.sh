#!/bin/sh

make -n -p "$@" | awk '
BEGIN {
    isTarget = 1
    isFile = 0
}
/^# Files/ {
    isFile = 1
}
/^# Not a target:/ {
    isTarget = 0
}
/^# VPATH Search Paths/ {
    isFile = 0
}
/^[^#: \t\/][^#: \t\/]*:/ {
    if (isFile && isTarget)
        targets[substr($0, 0, index($0, ":")-1)] = 1
    isTarget = 1
}
END {
    for (i in targets)
    	print i
}'
