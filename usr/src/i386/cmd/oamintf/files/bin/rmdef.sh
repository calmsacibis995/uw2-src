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

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/rmdef.sh	1.2"
#ident	"$Header: $"

DEV=$1
FS=$2
> /tmp/vfstab.${VPID}
> /tmp/remove.${VPID}
if [ ! -b $DEV ]
then
	BDEVICE=`devattr "$DEV"  bdevice 2>/dev/null`
else
	BDEVICE="$DEV"
fi
if [ "$FS" = "ALL" ]
then
	grep -v "$BDEVICE" /etc/vfstab >> /tmp/vfstab.${VPID} 2>&1
	if [ "$?" -eq 0 ]
	then
		echo "The defaults for the $BDEVICE device have
been removed for $FS mount point(s)." > /tmp/remove.${VPID}
	fi
else
	while read bdev rdev mountp fstype fsckpass automnt mntopts
	do
		if test "$BDEVICE" != "$bdev" -o "$FS" != "$mountp"
		then
			echo $bdev $rdev $mountp $fstype $fsckpass $automnt $mntopts | awk '{printf("%-17s %-21s %-6s %-6s %-1s %-3s %-8s\n", $1, $2, $3, $4, $5, $6, $7)}' >> /tmp/vfstab.${VPID}
		fi
	done < /etc/vfstab
	linold=`wc -l /etc/vfstab`
        linnew=`wc -l /tmp/vfstab.${VPID}`
        if [ "$linold" -eq "$linnew" ]
	then
		echo "ERROR: $FS is not a valid file system mount
point for the $BDEVICE device.  You may have
the wrong device name or mount point name.  
Please try again after you have updated the
form. Press CMD-MENU (AFTER RETURNING TO THE FORM)
in order to update the form." > /tmp/remove.${VPID}
	else
		echo "The defaults for the $BDEVICE device on the
$FS mount point have been removed." > /tmp/remove.${VPID}
	fi
fi
$TFADMIN mv /tmp/vfstab.${VPID} /etc/vfstab
exit 0
