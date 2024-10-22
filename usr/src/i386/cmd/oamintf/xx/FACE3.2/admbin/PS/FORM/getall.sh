#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/getall.sh	1.1.1.2"
#ident	"$Header: $"
/usr/vmsys/admin/PS/FORM/gform $1 $2 
exist=`/usr/bin/grep "^$1:" /usr/vmsys/OBJECTS/PS/FORM/alnames 2>/dev/null | /usr/bin/cut -d' ' -f2` 
if [ ! -z "$exist" ]
then
	echo pattern=$exist >> /usr/tmp/form.$VPID
fi
if [ -s /usr/spool/lp/admins/lp/forms/$1/allow ]
	then
	permit=`/usr/bin/awk '{print $1 }' /usr/spool/lp/admins/lp/forms/$1/allow `
elif [ -f /usr/spool/lp/admins/lp/forms/$1/deny ]
	then
	echo 
	permit=all 
elif [ ! -f /usr/spool/lp/admins/lp/forms/$1/deny ]
	then
	permit=none 
fi
echo users=$permit >> /usr/tmp/form.$VPID
