#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/deffs.sh	1.1.5.2"
#ident  "$Header: deffs.sh 2.0 91/07/12 $"
while read fsys dummy  fsname dummy
do
	case $fsys in
	'#'* | '')
		continue
	esac
	case $fsname in
	'-')
		continue
	esac

	echo "${fsname}"
done < /etc/vfstab 
