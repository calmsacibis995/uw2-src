#ident	"@(#)stand:i386sym/standalone/i386/symbols.awk	1.1"

BEGIN {
	name=""
}
/^[a-zA-Z_][a-zA-Z0-9_]*:/ {
	name = $1
	next
}
name != "" {
	printf "\t.set\t%s,%s\n",substr(name,1,length(name)-1),$2
	name = ""
}
