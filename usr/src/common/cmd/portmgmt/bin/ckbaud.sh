#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/ckbaud.sh	1.2.7.3"
#ident  "$Header: ckbaud.sh 2.0 91/07/13 $"

# ckbaud - check if the baud rate is available
#	   (ttyvalues contains a list of valid baud rates)
#	   Input:	$1 - autobaud flag (yes/no)
#			$2 - baud rate
#			$3 - name of file containing baud rate info

OK=0		
NOTEXIST=1	# not exist
NOTVALID=2	# not valid
NOTHING=3	# nothing entered or wrong args

case $1 in
	yes)
		test -z "$2" && exit $OK
		exit $NOTVALID;;
	no)	
		test -z "$2" && exit $NOTHING
		if [ "$2" = 0 ]
		then
			exit $NOTEXIST
		fi
		speed=`grep "^$2	" $3 2>/dev/null`
		if test -z "$speed"
		then
			exit $NOTEXIST
		fi
		exit $OK;;
	*)	exit $NOTHING;;
		
esac
