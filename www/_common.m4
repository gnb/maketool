define(DATE,esyscmd(date +"%d %b %Y"))dnl
define(YEAR,esyscmd(date +"%Y"))dnl
define(BEGINHEAD,
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML><HEAD>
include(_copyright.html)
<title>TITLE</title>
)dnl
define(ENDHEAD,
</HEAD>
)dnl
define(BEGINBODY,
<BODY BGCOLOR="#ffffff">
<TABLE BORDER=0 CELLSPACING=4>
  <TR>
    <TD COLSPAN=2 ALIGN=CENTER>
      <IMG WIDTH=430 HEIGHT=80 SRC="maketool_n.gif" ALT="maketool">
    </TD>
  </TR>
  <TR>
    <TD VALIGN=TOP>
include(toc.html.in)
    </TD>
    <TD>
    <CENTER><H1>TITLE</H1></CENTER>
)dnl
define(ENDBODY,
    </TD>
  </TR>
</TABLE>
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
dnl Usage: BEGINDOWNLOAD ( DOWNLOAD(description,filename) )* ENDDOWNLOAD
define(BEGINDOWNLOAD,
<table>
)dnl
define(ENDDOWNLOAD,
</table>
)dnl
define(DOWNLOAD,
`  <tr>
    <td valign=top><b>$1</b></td>
    <td>
      <a href="$2">$2</a><br>
      esyscmd(ls -l RELEASEDIR/$2 | awk {print`\$'5}) bytes<br>
      MD5 esyscmd(md5sum RELEASEDIR/$2 | awk {print`\$'1})
    </td>
  </tr>'
)dnl
