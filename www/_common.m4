define(DATE,esyscmd(date +"%d %b %Y"))dnl
define(YEAR,esyscmd(date +"%Y"))dnl
define(BEGINHEAD,
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html><head>
include(_copyright.html)
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title>TITLE</title>
)dnl
define(ENDHEAD,
</head>
)dnl
define(BEGINBODY,
<body>
<table border="0" cellspacing="4">
  <tr>
    <td colspan="2" align="center">
      <img width="430" height="80" src="maketool_n.gif" alt="maketool">
    </td>
  </tr>
  <tr>
    <td valign="top">
include(toc.html.in)
    </td>
    <td>
    <center><h1>TITLE</h1></center>
)dnl
define(ENDBODY,
    </td>
  </tr>
</table>
<!-- <hr noshade size=4> -->
<br><br>
<center>
<p class="small">
Last updated: DATE.
<br>
Copyright &copy; 1999-YEAR Greg Banks. All Rights Reserved.
<br>
</p>
</center>
</body></html>
)dnl
define(EMAILME,
$1 <a href="mailto:gnb@alphalink.com.au?Subject=maketool">gnb@alphalink.com.au</a>
)dnl
dnl Usage: THUMBNAIL(some/dir/fred,gif,[alternate text])
define(THUMBNAIL,
<a href="$1.$2"><img src="$1_t.$2" alt="$3" border="0"></a>
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
    <td valign="top"><b>$1</b></td>
    <td>
      <a href="$2">$2</a><br>
      esyscmd(ls -lL downloads/$2 | awk {print`\$'5}) bytes<br>
      MD5 esyscmd(md5sum downloads/$2 | awk {print`\$'1})
    </td>
  </tr>'
)dnl
define(BEGINTHANKS,
<table>
)dnl
define(ENDTHANKS,
</table>
)dnl
define(THANKS,
`  <tr>
    <td><i>$1</i></td>
    <td>$2</td>
  </tr>'
)dnl
