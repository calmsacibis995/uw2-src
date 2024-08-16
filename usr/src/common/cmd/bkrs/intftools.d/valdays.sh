#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/valdays.sh	1.4.6.2"
#ident  "$Header: valdays.sh 1.2 91/06/21 $"
# script passes one comma-separated list to the validation routine
if [ "$1" = "$2" ]
then
	exit 0
else
	DAYS=`echo "$1" | sed -e "s/  */,/g"`
	validdays $DAYS
	exit $?
fi
exit 0
