#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkmadd.sh	1.3.6.2"
#ident  "$Header: bkmadd.sh 1.2 91/06/21 $"
# script to add a line to crontab to schedule a reminder to back up
# file systems and data partitions.
# Lines are tagged to allow easy identification for later changes
# or removal.
HOUR=`echo "$1" | cut -f1 -d:`
MIN=`echo "$1" | cut -f2 -d:`
DAYS=`echo "$2" | sed -e "s/  */,/g"`
MONTHS=`echo "$3" | sed -e "s/  */,/g"`
ONAMES=`echo "$4" | sed -e "s/  */,/g"`
FILE=/tmp/bkrs$$

if [ "$DAYS" = "all" ]
then
	DAYS=\*
fi

if [ "$MONTHS" = "all" ]
then
	MONTHS=\*
fi

LINE="$MIN $HOUR * $MONTHS $DAYS /usr/sadm/bkup/bin/bkmsg $ONAMES #bkmsg#"

crontab -l >$FILE
echo "$LINE" >>$FILE
crontab <$FILE 2>/dev/null
RC=$?
rm $FILE
exit $RC
