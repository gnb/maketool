#!/bin/sh
#
# Maketool - GTK-based front end for gmake
# Copyright (c) 1999 Greg Banks
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# $Id: extract_targets.sh,v 1.3 1999-05-30 11:24:39 gnb Exp $
#

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
