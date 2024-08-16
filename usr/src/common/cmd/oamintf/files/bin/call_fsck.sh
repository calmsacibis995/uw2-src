#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/call_fsck.sh	1.2.3.2"
#ident  "$Header: call_fsck.sh 2.0 91/07/12 $"
$TFADMIN /usr/sbin/fsck -F $1 -m $2 1>/tmp/make.out 2>&1
if [ $? -eq 0 ]
then
	echo 0
else
	echo 1
fi
