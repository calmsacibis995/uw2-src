#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/getdeffs.sh	1.1.5.3"
#ident  "$Header: getdeffs.sh 2.0 91/07/12 $"
echo "ALL"
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
	if [ -n "$1" ] && [ "$1" != ALL ]
	then 
		if [ "${fsys}" = "$1" ]
		then
			echo "${fsname}"
		fi
	else
		echo "${fsname}"
	fi
done < /etc/vfstab 
