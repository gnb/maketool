/*
 * Maketool - GTK-based front end for gmake
 * Copyright (c) 1999-2003 Greg Banks
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"

/*
 * This file contains gettext string markup for common
 * autoconf option help strings.  It is never compiled,
 * only parsed during update-po.
 */
 
const char *s[] = {

/* cache-file */
N_("Cache test results in FILE"),
/* no-create */
N_("Do not create output files"),
/* prefix */
N_("Install architecture-independent files in PREFIX"),
/* exec-prefix */
N_("Install architecture-dependent files in EPREFIX"),
/* bindir */
N_("User executables in DIR"),
/* sbindir */
N_("System admin executables in DIR"),
/* libexecdir */
N_("Program executables in DIR"),
/* datadir */
N_("Read-only architecture-independent data in DIR"),
/* sysconfdir */
N_("Read-only single-machine data in DIR"),
/* sharedstatedir */
N_("Modifiable architecture-independent data in DIR"),
/* localstatedir */
N_("Modifiable single-machine data in DIR"),
/* libdir */
N_("Object code libraries in DIR"),
/* includedir */
N_("C header files in DIR"),
/* oldincludedir */
N_("C header files for non-gcc in DIR"),
/* infodir */
N_("Info documentation in DIR"),
/* mandir */
N_("Man documentation in DIR"),
/* srcdir */
N_("Find the sources in DIR"),
/* program-prefix */
N_("Prepend PREFIX to installed program names"),
/* program-suffix */
N_("Append SUFFIX to installed program names"),
/* program-transform-name */
N_("Run sed PROGRAM on installed program names"),
/* build */
N_("Configure for building on BUILD"),
/* host */
N_("Configure for HOST"),
/* target */
N_("Configure for TARGET"),
/* x-includes */
N_("X include files are in DIR"),
/* x-libraries */
N_("X library files are in DIR"),
/* disable-nls */
N_("Do not use Native Language Support"),
/* with-included-gettext */
N_("Use the GNU gettext library included here"),
/* with-catgets */
N_("Use catgets functions if available"),
/* with-gtk-prefix */
N_("Prefix where GTK is installed (optional)"),
/* with-gtk-exec-prefix */
N_("Exec prefix where GTK is installed (optional)"),
/* disable-gtktest */
N_("Do not try to compile and run a test GTK program"),

0};
