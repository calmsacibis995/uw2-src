#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/vallpi.sh	1.1.1.2"
#ident	"$Header: $"

if [ -z "$1" ]
	then echo "Input must be an integer of 2 digits or less." > /usr/tmp/err.$VPID
	echo false
elif [ "$1" = "0" ] ; then
	echo "$1 is not valid for lines per inch. (See the printer manual.)" > /usr/tmp/err.$VPID
	echo false
else 
	/usr/vmsys/admin/PS/FORM/alphanum "$1"
	if [ $? = 0 ]
		then echo true
	else
		echo "$1 is not valid for lines per inch. (See the printer manual.)" > /usr/tmp/err.$VPID
		echo false
	fi
fi
