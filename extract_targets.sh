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
    	print substr($0, 0, index($0, ":")-1)
    isTarget = 1
}' | sort -u
