#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/valfreq.sh	1.1.1.2"
#ident	"$Header: $"

if [ ! -z "$1" -a "$2" = "none" ]
	then 
echo "Since field \"Mount form alert \" is empty, the current field must be empty." > /usr/tmp/err.$VPID
	echo false
elif [ -z "$1" -a "$2" = "none" ]
	then echo true
else
	if [ "$1" = "once" -o "$1" = "1" -o "$1" = "5" -o "$1" = "30" -o "$1" = "60" ]
	then
		echo true
	else
		echo "$1 is not a valid frequency. Strike CHOICES for valid choices." > /usr/tmp/err.$VPID
		echo false
	fi
	
fi
