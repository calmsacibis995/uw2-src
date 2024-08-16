#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/ifdiskette.sh	1.1.6.3"
#ident  "$Header: ifdiskette.sh 2.0 91/07/12 $"
# usage: ifdiskette special
# returns true if diskette else returns false
DEVICE=$1
TYPE=`/usr/bin/devattr $DEVICE type 2> /dev/null`
if [ "$TYPE" = diskette ]
then
 	echo "true"
	exit 0
else
	echo "false"
	exit 1
fi
