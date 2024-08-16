#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#ident	"@(#)dstime:dstime/dst_sched.sh	1.2"
#
CMDPATH=/usr/lib/dstime
TMPFILE1=/tmp/$$dst_next
TMPFILE2=/tmp/$$atscript

trap "trap 0;rm -f $TMPFILE1 $TMPFILE2" 1 2 3 15 24
$CMDPATH/dst_next >$TMPFILE1
if [ $? -ne 0 ]
then
	rm -f $TMPFILE1 $TMPFILE2
	exit 1
fi
. $TMPFILE1

LINE1="$CMDPATH/dst_sched"
LINE2="$CMDPATH/dst_adtodc $ADJUSTMENT $CORRECTION"
LINE3="$CMDPATH/dst_pgen TZ_OFFSET $TZ_OFFSET"
echo "$LINE1\n$LINE2\n$LINE3" >$TMPFILE2
[ $? -eq 0 ] || exit 1

OLD_LANG=$LANG
LANG=C
export LANG
JOBNO=`at -f $TMPFILE2 $SCHEDDATE + 5 min 2>&1`
RETCODE=$?
if [ $RETCODE -eq 0 ]
then
	JOBNO=`echo "$JOBNO" | \
		sed -n 's/^.*[Jj][Oo][Bb][ 	][ 	]*\([^ 	]*\).*$/\1/p'`
	echo "$JOBNO" >$CMDPATH/jobno
fi

[ -n "$OLD_LANG" ] && LANG="$OLD_LANG"
rm -f $TMPFILE1 $TMPFILE2
exit $RETCODE
