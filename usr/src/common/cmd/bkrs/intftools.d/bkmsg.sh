#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkmsg.sh	1.1.6.2"
#ident  "$Header: bkmsg.sh 1.2 91/06/21 $"
# script prints backup reminder message to standard output.  It
# is invoked with the list of file systems to remind about (comma-separated)
# or the keyword "all".

FSYS=$1

if [ "$FSYS" = "all" ]
then
	echo "Reminder: it is time to back up all file systems and data partitions."
else
	PRFSYS=`echo $FSYS | sed -e "s/,/, /g"`
	echo Reminder: it is time to back up "$PRFSYS".
fi
exit 0
