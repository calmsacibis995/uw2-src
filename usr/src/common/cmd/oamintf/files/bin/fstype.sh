#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/fstype.sh	1.1.4.1"
#ident	"$Header: $"
# fstype <mountp> <special>
# returns the type of special from vfstab (if any)
MOUNTP=$1
SPECIAL=$2
while read special dummy1 mountp fstype dummy2
do
	if [ "t$SPECIAL" = "t$special" -a "t$MOUNTP" = "t$mountp" ]
	then
		echo $fstype
		exit 0
	fi
done < /etc/vfstab
