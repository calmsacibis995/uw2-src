#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/valdeffs.sh	1.1.6.2"
#ident  "$Header: valdeffs.sh 2.1 91/09/12 $"

FS=$1
#search vfstab for file system name, if found success else failure
if test "$FS" = "ALL"
then
	echo "true"
	exit 0
fi
# we dominate /etc/vfstab, only reading it - no privs required
while read fsys dummy mountp dummy
do
	case "$mountp" in
	'-') 
		continue;;
	'mountp' )
		continue
	esac
	if test "$mountp" = "$FS"
	then
		echo "true"
		exit 0
	fi
done < /etc/vfstab
echo "false"
exit 1
