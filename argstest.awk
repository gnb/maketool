BEGIN {
    args = ""
    name = ""
    in_exp = 0
    expfile = "argstest.exp"
    system("rm -f "expfile)
}
/^#/ {
    next
}
/^@name[ \t]/ {
    if (in_exp) {
    	print "Testing "name"..."
	status = system("./maketool "args" | diff -u "expfile" -")
	close(expfile) ; system("rm -f "expfile)
	in_exp = 0
	if (status) exit(status)
    }
    name = substr($0, 7, length($0))
    next
}
/^@args[ \t]/ {
    args = substr($0, 7, length($0))
    next
}
/^@exp/ {
    in_exp = 1
    printf "" > expfile
    next
}
{
    if (in_exp)
    	print $0 >> expfile
}
END {
    if (in_exp) {
    	print "Testing "name"..."
	status = system("./maketool "args" | diff -u "expfile" -")
	close(expfile) ; system("rm -f "expfile)
	if (status) exit(status)
    }
}
