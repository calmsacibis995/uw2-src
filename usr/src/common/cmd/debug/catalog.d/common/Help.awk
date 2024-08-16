#ident	"@(#)debugger:catalog.d/common/Help.awk	1.2"

# This awk script reads the file of Command Line Interface help messages (cli.help)
# and generates a C file containing the table of help messages that
# is used by Help.C

BEGIN {
	in_help = 0
	at_start = 1
	FS = " "
}

{

	if (at_start)
	{
		if (threads == 1)
			printf "#ifdef DEBUG_THREADS\n"
		else
			printf "#ifndef DEBUG_THREADS\n"
		printf "const char *Help_msgs[] = \n{\n0,\n"
		at_start = 0
	}
	if ($1 == "++")
	{
		if (in_help)
		{
			in_help = 0
			printf "\",\n\n"
		}
		else
		{
			printf "\"\\n\\\n"
			in_help = 1
		}
	}
	else if (in_help)
	{
		n = split($0, a, "\\")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\\\" a[i]

		n = split(newstr, a, "\"")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\\"" a[i]

		n = split(newstr, a, "\t")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\t" a[i]

		printf "%s\\n\\\n", newstr
	}
}

END {
	printf "};\n"
	printf "#endif\n"
}
