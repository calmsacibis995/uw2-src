#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/full.sh	1.5.6.2"
#ident  "$Header: full.sh 1.2 91/06/21 $"

WEEKS=$2
DAYS=$3
OPTS="-A -v -t $1"
TFILE=/tmp/bkreg$$

if [ "$2" != "all" -o "$3" != "all" ]
then
	if [ "$2" = "all" ]
	then
		WEEKS="1-52"
	fi

	if [ "$3" = "all" ]
	then
		DAYS="0-6"
	fi

	if [ "$2" = "demand" ]
	then
		SELECT=$WEEKS
	else
		SELECT="$WEEKS:$DAYS"
	fi
	OPTS="$OPTS -c \"$SELECT\""
fi

eval bkreg $OPTS >$TFILE 2>&1
RC=$?
echo $TFILE
exit $RC
