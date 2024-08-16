#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/getumntdev.sh	1.1.2.1"
#ident	"$Header: $"

/sbin/mount | /usr/bin/cut -d" " -f1,2,3 | \
while read MNTNAME ON DEVNAME
do
	nam=`echo "$DEVNAME" | sed 's/^.*dsk\///'`
	chr=`echo "$nam" | sed 's/^\(.\).*/\1/'`
	if [ "$chr" = "c" ]
	then
		slice=`echo "$nam" | sed 's/^.*s//'`
		device=`echo "$nam" | sed 's/s.*/s0/'`
		DESC="`devattr /dev/rdsk/"$device" alias` slice $slice"
	else
		DESC="$DEVNAME"
	fi
	echo "$MNTNAME\072$ON $DESC"
done

