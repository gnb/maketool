
Maketool
========

This is a rather old open source project of mine, which I recently dug
out from backup media and converted from CVS to git.  As you can see
from the logs, I haven't done any actual work on this in about nine
years.  Perhaps it might be useful to you...good luck!

Maketool a simple GTK+ based GUI front end for GNU make and
other make programs.

Maketool's features include:

* Figures out what targets are available and presents them in a menu.
* Runs make and detects compiler errors and warnings in the output.
* Double-clicking on errors starts an editor with that file and line.
* Works with any makefile system that uses GNU make.
* Automatically updates Makefile if project uses automake, autoconf or imake.
* GUI for selecting configure options for automake and autoconf.
* Special handling for standard GNU targets such as _all_,
  _install_, _clean_ etc.
* Save and reload logs of make runs.
* Summarise make logs to reduce visual clutter.
* With recursive make, expand or collapse each directory separately
* Override values of make and environment variables.
* Make in series or in parallel, (using make's -j and -l flags).
* Dryrun mode (using make's -n flag).
* Implements many of make's command-line options.
* Configure foreground & background colours of error & warnings lines.
* Now also supports Solaris make, IRIX smake, and BSD pmake.

