#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkssel.sh	1.2.6.2"
#ident  "$Header: bkssel.sh 1.2 91/06/21 $"
# shell invokes bkstatus with selective options to filter
# output.
STATES="$1"
JOBS="$2"
USERS="$3"
OPTS=
ST=
TFILE=/tmp/bks$$

if [ "$STATES" != "" ]
then
	if [ "$STATES" = "all" ]
	then
		OPTS=-a
	else 
		for i in $STATES
		do
			case $i in
			active)		ST=${ST}a
					;;
			completed)	ST=${ST}c
					;;
			failed)		ST=${ST}f
					;;
			pending)	ST=${ST}p
					;;
			suspended)	ST=${ST}s
					;;
			waiting)	ST=${ST}w
					;;
			esac
		done

		ST="-s $ST"
	fi
fi

if [ "$JOBS" != "all" ]
then
	OPTS="-j \"$JOBS\""
fi

if [ "$USERS" != "all" ]
then
	OPTS="$OPTS -u \"$USERS\""
fi

(eval bkstatus $OPTS $ST) >$TFILE 2>&1
RC=$?
echo $TFILE
exit $RC
