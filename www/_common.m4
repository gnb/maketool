define(DATE,esyscmd(date +"%d %b %Y"))dnl
define(YEAR,esyscmd(date +"%Y"))dnl
define(BEGINHEAD,
<HTML><HEAD>
include(_copyright.html)
<title>TITLE</title>
)dnl
define(ENDHEAD,
</HEAD>
)dnl
define(BEGINBODY,
<BODY BGCOLOR="#ffffff">
<CENTER><H1>TITLE</H1></CENTER>
)dnl
define(ENDBODY,
<!-- <HR NOSHADE SIZE=4> -->
<BR><BR>
<CENTER>
<P><FONT SIZE=-1>Last updated: DATE.<BR>
&copy; 1997-YEAR Greg Banks. All Rights Reserved.<BR>
</FONT></P>
</CENTER>
</BODY></HTML>
)dnl
define(EMAILME,
$1 <A HREF="mailto:gnb@alphalink.com.au?Subject=maketool">gnb@alphalink.com.au</A>
)dnl
dnl Usage: THUMBNAIL(some/dir/fred,gif,[alternate text])
define(THUMBNAIL,
<A HREF="$1.$2"><IMG SRC="$1_t.$2" ALT="$3" BORDER=0></A>
)dnl
