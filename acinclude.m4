dnl 
dnl This file is included along with aclocal.m4 when building configure.in
dnl
dnl Maketool - GTK-based front end for gmake
dnl Copyright (c) 1999-2003 Greg Banks
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl 
dnl $Id: acinclude.m4,v 1.1 2003-05-13 01:14:34 gnb Exp $
dnl

dnl For gcc, ensure that the given flags are in $CFLAGS
AC_DEFUN([AC_GCC_ADD_CFLAGS],[
if test "x$GCC" = "xyes"; then
    AC_MSG_CHECKING([for additional gcc flags])
    CEXTRAWARNFLAGS=
    for flag in `cat <<EOF
$1
EOF
` _dummy
    do
    	test $flag = _dummy && continue
	case " $CFLAGS " in
	*[[\ \	]]$flag[[\ \	]]*) ;;
	*) CEXTRAWARNFLAGS="$CEXTRAWARNFLAGS $flag" ;;
	esac
    done
    CFLAGS="$CFLAGS $CEXTRAWARNFLAGS"
    AC_MSG_RESULT([$CEXTRAWARNFLAGS])
fi
])dnl

dnl For gcc, ensure that the given flags are not in $CFLAGS
AC_DEFUN([AC_GCC_REMOVE_CFLAGS],[
if test "x$GCC" = "xyes"; then
    AC_MSG_CHECKING([for spurious gcc flags])
    REMOVEDCFLAGS=
    CFLAGS=`echo "$CFLAGS" | sed -e 's|^[[\ \	]]*||' -e 's|[[\ \	]]*$||'`
    for flag in `cat <<EOF
$1
EOF
` _dummy
    do
    	test $flag = _dummy && continue
    	CFLAGS_tmp=`echo " $CFLAGS " | sed -e "s|[[\ \	]]$flag[[\ \	]]| |g" -e 's|^[[\ \	]]*||' -e 's|[[\ \	]]*$||'`
	if test "$CFLAGS_tmp" != "$CFLAGS" ; then
	    REMOVEDCFLAGS="$REMOVEDCFLAGS $flag"
	    CFLAGS="$CFLAGS_tmp"
	fi
    done
    AC_MSG_RESULT([$REMOVEDCFLAGS])
fi
])dnl

dnl ===============================
dnl
dnl Test for BSD vs SysV signal semantics
dnl
AC_DEFUN([AC_HAVE_SYSV_SIGNAL],[
AC_MSG_CHECKING(for signal semantics)
AC_TRY_RUN([
#include <signal.h>

static int ncaught;

static RETSIGTYPE
handler(int sig)
{
    ncaught++;
}

int
main(int argc, char **argv)
{
    int i;
    

#ifdef SIGCHLD
    signal(SIGCHLD, handler);
#else
    signal(SIGCLD, handler);
#endif
    for (i=0 ; i<5 ; i++)
	kill(getpid(), SIGCLD);

    switch (ncaught)
    {
    case 1: /* SysV signal semantics */
    	return 0;
    case 5: /* BSD signal semantics */
    	return 1;
    default: /* something is awry */
    	return 2;
    }
}
],AC_DEFINE(HAVE_SYSV_SIGNAL,1) AC_MSG_RESULT(sysv),
AC_DEFINE(HAVE_SYSV_SIGNAL,0) AC_MSG_RESULT(bsd),
AC_DEFINE(HAVE_SYSV_SIGNAL,0) AC_MSG_RESULT(assuming bsd))
])dnl
