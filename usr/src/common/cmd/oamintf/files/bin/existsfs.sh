#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/existsfs.sh	1.1.6.2"
#ident  "$Header: existsfs.sh 2.0 91/07/12 $"
DEVICE=$1
FSTYPE=$2
if [  ! -b $DEVICE ]
then
	DEVICE=`/usr/bin/devattr $DEVICE bdevice 2> /dev/null`
fi
if fsck -F $FSTYPE -m $DEVICE 2> /dev/null
then
	echo "true"
	exit 0
else
	echo "false"
	exit 1
fi
