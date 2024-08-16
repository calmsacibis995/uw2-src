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

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/mkfs_bfs.sh	1.2.3.4"
#ident  "$Header: mkfs_bfs.sh 1.1 91/09/17 $"

DEVICE=$1
BLOCKS=$2
INODES=$3
LABEL=$4
MOUNTP=$5
echo "" > /tmp/make.out
if [ ! -b $DEVICE ]
then
	BDEVICE=`/usr/bin/devattr "$DEVICE" bdevice 2>/dev/null`
else
	BDEVICE=$DEVICE
fi
if $TFADMIN /sbin/mkfs -F bfs $BDEVICE $BLOCKS:$INODES 2>/tmp/mkerr$$
then
	echo "The file system was created successfully." >> /tmp/make.out
	/bin/rm /tmp/mkerr$$ 2>/dev/null
else
	echo "The file system could not be created:\n" >> /tmp/make.out
	/usr/bin/cat /tmp/mkerr$$ >> /tmp/make.out
	/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
	exit 1
fi

if [ "$LABEL" != NULL ]
then
	echo "The label option is not applicable to a 'bfs' file system." >> /tmp/make.out
fi

if [ "$MOUNTP" != "" ]
then
	adddef $BDEVICE $MOUNTP "yes" "bfs" "" ""
	if $TFADMIN /sbin/mount -F bfs $BDEVICE $MOUNTP 2> /tmp/mnterr$$
	then
		putdev -m $DEVICE dparttype=fs fstype=bfs mountpt="$MOUNTP" nblocks="$BLOCKS" > /dev/null
		echo "File system successfully mounted as $MOUNTP." >> /tmp/make.out
		/bin/rm /tmp/mnterr$$ 2>/dev/null
	else
		echo "File system could not be mounted as \"$MOUNTP\":" >> /tmp/make.out
		/usr/bin/cat /tmp/mnterr$$ >> /tmp/make.out
		/usr/bin/rm /tmp/mnterr$$ 2>/dev/null
	fi
else
	adddef $BDEVICE "-" "no" "bfs" "" ""

fi
exit 0
