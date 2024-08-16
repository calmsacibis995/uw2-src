#ident	"@(#)kern-i386at:psm/ast/symbols_ast.awk	1.1"

BEGIN {
	name=""
}
/^[a-zA-Z_][a-zA-Z0-9_]*:/ {
	name = $1
	next
}
name != "" {
	printf "#define\t%s\t[%s]\n",substr(name,1,length(name)-1),$2
	name = ""
}
