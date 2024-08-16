#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)winxksh:xksh/readonly.sh	1.3"

# Make readonly data into text for sharing

error=0
CC=${PFX}cc
flags=
while test '' != "$1"
do	case $1 in
	-*) flags="$flags $1";;
	*)  break;;
	esac
	shift
done
for i in "$@"
do	x=`basename "$i" .c`
	${CC} ${CCFLAGS} $flags -S "$i"
	sed -e 's/^\([ 	]*\.*\)data/\1text/
		s/\([^:][ 	]*\.*\)zero[ 	][ 	]*/\1set	.,.+/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*4$/\1byte 0,0,0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*3$/\1byte 0,0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*2$/\1byte 0,0/
		s/\([^:][ 	]*\.*\)space[ 	][ 	]*1$/\1byte 0/' "$x.s" > data$$.s
	mv data$$.s "$x.s"
	if	${CC} -c "$x.s"
	then	rm -f "$x.s"
	else	error=1
		${CC} ${flags} -c "$i"
	fi
done
exit $error
