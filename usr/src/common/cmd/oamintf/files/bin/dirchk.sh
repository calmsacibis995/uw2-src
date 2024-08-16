#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/dirchk.sh	1.1.7.2"
#ident  "$Header: dirchk.sh 2.1 91/09/12 $"

file=${1}
if test -d ${file}
then
	exit 0
else
	echo "   $file is not the full path name of a valid directory."
	exit 1
fi
