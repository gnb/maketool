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
AC_DEFUN([AC_SIGNAL_SEMANTICS],[
AC_CACHE_CHECK(for signal semantics,ac_cv_signal_semantics,[


HAVE_BSD_SIGNAL=unknown
HAVE_BSD_SIGACTION=unknown
HAVE_BSD_SIGVEC=unknown

dnl First, test for BSD vs SysV signal semantics
dnl with the old-fashioned signal(3).

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
    
    signal(SIGCLD, handler);
    for (i=0 ; i<5 ; i++)
	kill(getpid(), SIGCLD);

    switch (ncaught)
    {
    case 5: /* BSD signal semantics */
    	return 0;
    case 1: /* SysV signal semantics */
    	return 1;
    default: /* something is awry */
    	return 2;
    }
}
],
HAVE_BSD_SIGNAL=yes,
HAVE_BSD_SIGNAL=no,
AC_MSG_ERROR(cannot handle cross-compilation, sorry))

if test $HAVE_BSD_SIGNAL = no ; then
dnl For SysV semantics, check if we can force the OS
dnl to implement BSD semantics with sigaction(2).
dnl This helps on OSes which don't automatically
dnl re-register but can't handle the app re-registering
dnl in the signal handler, e.g. Solaris 2.5.1.
AC_TRY_RUN([
#include <signal.h>

static int ncaught;

static void
handler(int sig)
{
    ncaught++;
}

struct sigaction act;

int
main(int argc, char **argv)
{
    int i;
    
    act.sa_handler = handler;
#ifdef SA_RESTART    
    act.sa_flags |= SA_RESTART;
#endif
    
    sigaction(SIGCLD, &act, 0);
    for (i=0 ; i<5 ; i++)
	kill(getpid(), SIGCLD);

    switch (ncaught)
    {
    case 5: /* BSD signal semantics */
     	return 0;
    case 1: /* SysV signal semantics */
    	return 1;
    default: /* something is awry */
    	return 2;
    }
}
],
HAVE_BSD_SIGACTION=yes,
HAVE_BSD_SIGACTION=no,
AC_MSG_ERROR(cannot handle cross-compilation, sorry))

dnl Just in case, try with sigvec(2) as well.
AC_TRY_RUN([
#include <signal.h>

static int ncaught;

static RETSIGTYPE
handler(int sig)
{
    ncaught++;
}

struct sigvec vec;

int
main(int argc, char **argv)
{
    int i;
    
    vec.sv_handler = handler;
    
    sigvec(SIGCLD, &vec, 0);
    for (i=0 ; i<5 ; i++)
	kill(getpid(), SIGCLD);

    switch (ncaught)
    {
    case 5: /* BSD signal semantics */
    	return 0;
    case 1: /* SysV signal semantics */
     	return 1;
    default: /* something is awry */
    	return 2;
    }
}
],
HAVE_BSD_SIGVEC=yes,
HAVE_BSD_SIGVEC=no,
AC_MSG_ERROR(cannot handle cross-compilation, sorry))

fi

case "${HAVE_BSD_SIGNAL},${HAVE_BSD_SIGACTION},${HAVE_BSD_SIGVEC}" in
yes,*) ac_cv_signal_semantics=bsd ;;
no,yes,*) ac_cv_signal_semantics=sigaction ;;
no,*,yes) ac_cv_signal_semantics=sigvec ;;
no,no,no) ac_cv_signal_semantics=sysv ;;
esac
])

SSEM=`echo $ac_cv_signal_semantics | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`

AC_DEFINE_UNQUOTED(SIGNAL_SEMANTICS,SIGNAL_SEMANTICS_$SSEM,[
 * Signal registration semantics.
 * BSD	    	BSD style, handlers are suppressed during
 *  	    	delivery but never unregistered so do not
 *  	    	need to re-regiser.
 * SIGACTION	registering using sigaction gives BSD semantics
 * SIGVEC   	registering using sigvec gives BSD semantics
 * SYSV     	SysV style, handler needs to be re-registered
 *  	    	after delivery.
])
])
