#ident	"@(#)terminfo:common/cmd/terminfo/Doc.sed	1.4.2.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/terminfo/Doc.sed,v 1.1 91/02/28 20:12:18 ccs Exp $"
#
#	This script is used to strip info from the terminfo
#	source files.
#
sed -n '
	/^# \{1,\}Manufacturer:[ 	]*\(.*\)/s//.M \1/p
	/^# \{1,\}Class:[ 	]*\(.*\)/s//.C \1/p
	/^# \{1,\}Author:[ 	]*\(.*\)/s//.A \1/p
	/^# \{1,\}Info:[ 	]*/,/^[^#][^	]/ {
		s/^# *Info:/.I/p
		/^#[	 ]\{1,\}/ {
			s/#//p
		}
		/^#$/ i\
.IE
	}
	/^\([^#	 ][^ 	]*\)|\([^|,]*\),[ 	]*$/ {
		s//Terminal:\
	"\2"\
		\1/
		s/|/, /g
		p
	}
' $*
