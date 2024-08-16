#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/invfstab.sh	1.1.6.3"
#ident  "$Header: invfstab.sh 2.0 91/07/12 $"
DEV=$1
if test -b $DEV
then
	BDEVICE="$DEV"
else
	BDEVICE=`/usr/bin/devattr "$DEV"  bdevice 2>/dev/null`
	if test "$BDEVICE" = ""
	then
		echo "false"
	 	exit 1	
	fi
fi
while read bdev rdev mountp fstype fsckpass automnt mntopts
do
	case $bdev in
	'#'* | '' )
		continue;;
	'-')
		continue
	esac
	if test "$BDEVICE" = "$bdev"
	then
		echo $fstype $mountp > /tmp/invfstab
		echo "true"
		exit 0
	else
		continue
	fi
done < /etc/vfstab
exit 1
