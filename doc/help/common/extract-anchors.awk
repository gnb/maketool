#!/usr/bin/awk -f
#
# Maketool - GTK-based front end for gmake
# Copyright (c) 1999-2003 Greg Banks
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
# $Id: extract-anchors.awk,v 1.1 2003-09-29 01:07:22 gnb Exp $
#

BEGIN {
    state = 0;
}
/<A$/ {
    state = 1;
#    printf "line %s:%d state=%d\n", FILENAME, NR, state;
    next;
}
/^NAME=".*"/ {
#    printf "line %s:%d state=%d\n", FILENAME, NR, state;
    if (state == 1) {
	state = 0;
#	printf "line %s:%d state=%d\n", FILENAME, NR, state;
    	tag = $0;
	gsub("^NAME=\"", "", tag);
	gsub("\".*$", "", tag);
	tag = tolower(tag);
	if (match(tag, "^aen[0-9][0-9]*$"))
	    next;
	if (match(tag, "^ftn\.aen[0-9][0-9]*$"))
	    next;
	if (match(tag, "-note-[0-9][0-9]*$"))
	    next;

    	fn = FILENAME;
	gsub("^.*/", "", fn);
	
	if (fn == tag".html")
	    printf "%-40s %s\n", tag, fn;
	else
	    printf "%-40s %s#%s\n", tag, fn, tag;
    }
}
{
    state = 0;
#    printf "line %s:%d state=%d\n", FILENAME, NR, state;
}
