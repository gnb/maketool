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
# $Id: extract_targets.sh,v 1.5 1999-06-01 13:19:05 gnb Exp $
#

make -n -p "$@" _no_such_target_ 2>/dev/null | awk '
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
/^[^#: \t][^#: \t]*:/ {
    if (isFile && isTarget) {
	targ = substr($0, 0, index($0, ":")-1)
	# Filter out some real but useless targets
        suitable = 1
	if (substr(targ, 0, 1) == ".")
	    suitable = 0
	if (index(targ, "/.") > 0)
	    suitable = 0
	if (substr(targ, 0, 1) == "/")
	    suitable = 0
	if (substr(targ, 0, 3) == "../")
	    suitable = 0
	if (suitable)
    	    print targ
    }
    isTarget = 1
}' | sort -u
