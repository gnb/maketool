#!/usr/bin/env python

import sys
import re
import os

def usage():
    "Print program usage and exit"
    print "Usage: " + sys.argv[0] + " LANG program [args...]"
    print "LANG is of the form aa[_BB[.EEE]] where"
    print "  aa = language, BB = territory, EEE = encoding"
    print "  e.g. ja_JA.EUC or cs_CZ"
    sys.exit(1)

if len(sys.argv) < 3:
    usage()
    
langspec = sys.argv[1]
# mo = re.compile("^([a-z][a-z])(_[A-Z][A-Z])?(\.[a-zA-Z][a-zA-Z0-9-_]+)?$").match(langspec)
# if mo == None:
#     print "Bad form for LANG"
#     usage()
#     
# language = mo.group(1)
# 
# # Get territory -- strip off leading underscore
# territory = mo.group(2)
# if territory != None:
#     territory = territory[1:]
# 
# # Get encoding -- strip off leading dot.
# encoding = mo.group(3)
# if encoding != None:
#     encoding = encoding[1:]
#
# print 'language="%s" territory="%s" encoding="%s"' % (language, territory, encoding)

os.environ['LC_ALL'] = langspec
os.environ['LC_MESSAGES'] = langspec
os.environ['LC_TIME'] = langspec
os.environ['LC_NUMERIC'] = langspec
os.environ['LC_CTYPE'] = langspec
os.environ['LC_MONETARY'] = langspec
os.environ['LC_COLLATE'] = langspec
os.environ['LANG'] = langspec
os.environ['LANGUAGE'] = langspec

os.execvp(sys.argv[2], sys.argv[2:])
