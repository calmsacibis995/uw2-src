#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/odm.sh	1.1"

typeset pwin

touch /etc/conf/pack.d/vxfs/Driver_vj.o
mkdir /etc/vx ; touch /etc/vx/upgrade
/usr/sbin/pkgrm -n onlinemgr

while :
do
	display "$ODM_PROMPT"
	pwin=$CURWIN
	OIFS=$IFS
	IFS="$nl"
	set -A ODM_UP_OPT ${ODM_MEDIA_CHOOSE}
	IFS="$OIFS"
	choose -e -f "$ODM_UP_OPT" "${ODM_UP_OPT[@]}"
	input_handler
	call ioctl 0 30213 2
	if [ "$CHOICE" = "${ODM_UP_OPT[0]}" ]
	then
		/usr/sbin/pkgadd -q -d cdrom1 odm </dev/vt02 >/dev/vt02 2>&1
		retval=$?
	else
		/usr/sbin/pkgadd -q -d diskette1 odm </dev/vt02 >/dev/vt02 2>&1
		retval=$?
	fi
	call ioctl 0 30213 1
	wclose $pwin
	stty min 1
	case $retval in
		0 | 10 | 20 )
			break	#out of while loop
			;;
	esac
done
