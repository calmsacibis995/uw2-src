#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/vcmount.sh	1.1.1.2"
#ident	"$Header: $"

if [ "$1" = "mail" -o "$1" = "none" -o -x "$1" ]
	then true
else
	if [ -f "$1" ]
		then echo "$1" is not an executable file. > /usr/tmp/err.$VPID
	elif [ ! -f "$1" ]
		then echo "$1" does not exist. > /usr/tmp/err.$VPID
	fi
	echo false
fi



