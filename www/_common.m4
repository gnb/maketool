define(DATE,esyscmd(date +"%d %b %Y"))dnl
define(YEAR,esyscmd(date +"%Y"))dnl
define(BEGINHEAD,
<HTML><HEAD>
<!-- (c) 1997-1999 Greg Banks. All Rights Reserved. -->
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
<A HREF="mailto:gnb@alphalink.com.au?Subject=maketool">$1</A>
)dnl
