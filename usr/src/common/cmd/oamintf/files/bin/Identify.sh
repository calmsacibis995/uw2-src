#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/Identify.sh	1.1.8.2"
#ident  "$Header: Identify.sh 2.0 91/07/12 $"
FSYS=`/usr/bin/devattr  ${1} bdevice 2> /dev/null`
if [ "t$FSYS" = "t" ]
then
	FSYS=$1
fi
echo "Possible type(s):\n\n" > /tmp/fstype
/usr/sbin/fstyp ${FSYS} >> /tmp/fstype.out
case $? in
	0) /usr/bin/cat /tmp/fstype.out >> /tmp/fstype;;
	1) echo "cannot identify file system type" >> /tmp/fstype;;
	2) /usr/bin/cat /tmp/fstype.out >> /tmp/fstype;
	  echo "\nWarning: more than one fstype identified.\nIf you are going to check this file system, you should\nselect the 'check only' option." >>/tmp/fstype;;
esac
/usr/bin/rm -f /tmp/fstype.out
echo "" >> /tmp/fstype
