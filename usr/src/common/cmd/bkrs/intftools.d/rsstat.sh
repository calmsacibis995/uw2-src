#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/rsstat.sh	1.3.6.2"
#ident  "$Header: rsstat.sh 1.2 91/06/21 $"
# script to invoke rsstatus with jobids, users and devices to filter
# output
JOBIDS="$1"
USERS="$2"
DTYPES="$3"
DLABELS="$4"
TFILE=/tmp/rss$$

if [ "$JOBIDS" != 'all' ]
then
	OPTS="-j \"$1\""
fi
if [ "$USERS" != 'all' ]
then
	OPTS="$OPTS -u \"$2\""
fi
if [ "$DTYPES" != 'all' ]
then
	DEV="$DTYPES"
fi
if [ "$DLABELS" != 'all' ]
then
	DEV="$DEV:$DLABELS"
fi
if [ "$DEV" != "" ]
then
	OPTS="$OPTS -d $DEV"
fi

eval rsstatus $OPTS >$TFILE 2>&1
RC=$?
echo $TFILE
exit $RC
