#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/ufs_fragsiz.sh	1.1.5.2"
#ident  "$Header: ufs_fragsiz.sh 2.0 91/07/12 $"
BLOCKSIZE=$1
if [ $BLOCKSIZE -eq 4096 ]
then
	echo 512
	echo 1024
	echo 2048
	echo 4096
else
	echo 1024
	echo 2048
	echo 4096
	echo 8192
fi
exit 0
	
