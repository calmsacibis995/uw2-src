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

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/mkfs_ufs.sh	1.4.5.4"
#ident  "$Header: mkfs_ufs.sh 2.0 91/07/12 $"
PROTO=$1
DEVICE=$2
BLOCKS=$3
BSIZE=$4
FRAGSIZ=$5
LABEL=$6
MOUNTP=$7
FLOPPY=$8
echo "" > /tmp/make.out
if [ ! -c $DEVICE ]
then
        if [ -s /var/tmp/make.$VPID ]
        then
                CDEVICE=`devattr "$DEVICE" cdevice 2>/dev/null`
        else
                echo "The file system could not be created:\nThe raw device must be specified." >> /tmp/make.out
                exit 1
        fi
else
        CDEVICE=$DEVICE
fi
if [ "$FLOPPY" = TRUE ]
then
	mkfsopts="ntrack=2,nsect=9,"
fi
	
if [ "$PROTO" != "NULL"  ]
then
	if  $TFADMIN /sbin/mkfs -F ufs -p $PROTO $CDEVICE >/tmp/mkerr$$ 2>&1
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		/usr/bin/cat /tmp/mkerr$$ >>/tmp/make.out
		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
		
	fi
else
	if $TFADMIN /sbin/mkfs -F ufs -o "${mkfsopts}"bsize="$BSIZE",fragsize="$FRAGSIZ"  "$CDEVICE" "$BLOCKS" >/tmp/mkerr$$ 2>&1
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		/usr/bin/cat /tmp/mkerr$$ >>/tmp/make.out
#		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
	fi
fi

if [ "$LABEL" != "NULL" ]
then
	/sbin/labelit -F ufs "$CDEVICE" "$LABEL" 2>/dev/null
	echo "The new file system has been labelled $LABEL." >> /tmp/make.out
fi

if [ "$MOUNTP" != "" ]
then
	if [ ! -b $DEVICE ]
        then
                if [ -s /var/tmp/make.$VPID ]
                then
                        BDEVICE=`devattr "$DEVICE" bdevice 2>/dev/null`
                else
                        echo "The file system cannot be mounted.\nThe
block device must be specified in device.tab file." >> /tmp/make.out
                        exit 1
                fi
        else
                BDEVICE=$DEVICE
        fi
        adddef $BDEVICE $MOUNTP "yes" "ufs" "" "no"
	if $TFADMIN /sbin/mount -F ufs $BDEVICE $MOUNTP 2> /dev/null
	then
		echo "The file system has been mounted as $MOUNTP." >> /tmp/make.out
	fi
	putdev -m $DEVICE dparttype=fs fstype=ufs mountpt="$MOUNTP" nblocks="$BLOCKS" > /dev/null
else
	adddef $DEVICE "-" "no" "ufs" "" "no"
fi
exit 0
