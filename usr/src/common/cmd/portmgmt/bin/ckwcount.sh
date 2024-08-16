#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/ckwcount.sh	1.1.6.3"
#ident  "$Header: ckwcount.sh 2.0 91/07/13 $"

# ckwcount - check if the wait read count is valid
#	   Input:	$1 - wait read flag (yes/no)
#			$2 - wait read count 

OK=0		
NOTVALIDYES=1	# not valid for waitread=yes
NOTVALIDNO=2	# not valid for waitread=no
NOTHING=3	# nothing entered or wrong args

case $1 in
	yes)
		test -z "$2" && exit $NOTVALIDYES
		num=`expr "$2" + 0 2>/dev/null`
		test -z "$num" && exit $NOTVALIDYES
		[ $num -lt 0 ] && exit $NOTVALIDYES
		exit $OK;;
	no)	
		test -z "$2" && exit $OK
		exit $NOTVALIDNO;;
	*)	exit $NOTHING;;
		
esac
