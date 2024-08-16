#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/labelfsname.sh	1.2.3.1"
#ident	"$Header: $"
#	given the output of /etc/labelit, return the file system name and
#	volume label as shell variable assignments.

#!	chmod +x ${file}

if [ $# -eq 0 ]
then
	echo >&2 "Usage:  $0 /etc/labelit-output"
	exit 1
fi

echo $*  |
	sed -n 's/Current fsname: \(.*\), Current volname: \(.*\), Blocks: .*/fsname=\1 label=\2/p'
