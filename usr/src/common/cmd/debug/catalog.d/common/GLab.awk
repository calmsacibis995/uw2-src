#ident	"@(#)debugger:catalog.d/common/GLab.awk	1.2"

# Label support for menu and dialog buttons, toggles, etc.
# This awk script reads the Label table description file GLab.awk.in
# and creates three new files:
#
# gui_label.h
#	enum declaration of the types of labels
#	There is one entry for each non-comment line in GLab.awk.in
#
# GLabel.C
#	The initialization code for the label table,
#	one line per Label type.  Each line looks like:
#		{catalog number, 0, default string}, /* LableId */
#
# GLcatalog
#	Label text, one per line.  This is the input to mkmsgs,
#	which transforms it into the gui-specific label catalog, 
#	debug.label
#

BEGIN {
	f_tab_c = "GLabel.C"
	f_msg_h = "gui_label.h"
	f_cat = "GLcatalog"

	# print the necesary header information to each file
	print "/* file produced by ../../catalog.d/common/GLab.awk */\n" >f_msg_h
	print "#ifndef _GUI_LABEL_H"	>f_msg_h
	print "#define _GUI_LABEL_H\n"	>f_msg_h
	print "enum LabelId\n{"		>f_msg_h
	print "\tLAB_none = 0,"	>f_msg_h

	print "/* file produced by ../../catalog.d/common/GLab.awk\n */"  >f_tab_c
	print "#include \"gui_label.h\"" > f_tab_c
	print "#include \"Label.h\"" > f_tab_c
	print "struct Label labtable[] = {" > f_tab_c
	print "{ 0, 0 },\t/* LAB_none */" >f_tab_c

	# ------ WARNING -------
	# Messages in an existing catalog cannot be modified or removed,
	# because we have no control over the translated catalogs,
	# also, calls to gettxt have hard-coded numbers in them.
	# Messages MUST stay in the same order.
	next_num = 1			# next catalog entry
}

# main loop
# the command line in the makefile is
# 	awk -f GLab.awk GLab.awk.in

{
	# The only lines we are interested in are the ones that
	# start with LAB_

	if (substr($1, 1, 4) != "LAB_")
		next

	msg_num = next_num++
	printf "\t%s,\n", $1 >f_msg_h
	printf "{ %d, 0, \t\"", msg_num >f_tab_c
	fstring = ""
	mid = $1

	# initial padding
	i = 2
	while($i == "\\")
	{
		i++
		fstring = fstring " "
	}
	# Labels may contain several words
	for (; i <= NF; i++)
	{
		if (fstring != "")
			fstring = fstring " "
		if ($i != "\\")	#\space used to ensure spacing
			fstring = fstring $i
	}
	printf "%s\"},\t/* %s */\n", fstring, mid > f_tab_c
	print fstring > f_cat
}

END {
	# finish off the message table files
	print "\tLAB_last\n};\n"	>f_msg_h
	print "#endif // _GUI_LABEL_H"	>f_msg_h

	print "{0}\t/* LAB_last */\n};" >f_tab_c
}
