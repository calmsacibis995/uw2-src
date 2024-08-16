#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/devtabcmd.sh	1.1.6.2"
#ident  "$Header: devtabcmd.sh 2.0 91/07/12 $"
CMD=`/usr/bin/devattr $1 $2`
if [ $? -ne 0 ]
then
	exit 1
fi
$CMD 2>/dev/null
if [ $? -ne 0 ]
then
	exit 1
else
	exit 0
fi
