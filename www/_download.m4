dnl Usage: BEGINDOWNLOAD ( DOWNLOAD(description,filename) )* ENDDOWNLOAD
define(BEGINDOWNLOAD,
<TABLE>
)dnl
define(ENDDOWNLOAD,
</TABLE>
)dnl
define(DOWNLOAD,
`  <TR>
    <TD VALIGN=TOP><B>$1</B></TD>
    <TD>
      <A HREF="$2">$2</A><BR>
      esyscmd(ls -l $2 | awk {print`\$'5}) bytes<BR>
      MD5 esyscmd(md5sum $2 | awk {print`\$'1})
    </TD>
  </TR>'
)dnl
