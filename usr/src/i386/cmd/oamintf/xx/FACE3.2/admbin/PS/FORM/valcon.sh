#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/valcon.sh	1.1.1.2"
#ident	"$Header: $"
if [ ! -z "$1" -a -z "$2" ]
	then 
echo "Since field \"Alignment pattern file\" is empty, the current field must be empty." > /usr/tmp/err.$VPID
	echo false
elif [ -z "$1" -a -z "$2" ]
	then echo true
elif [ -z "$1" -a ! -z "$2" ]
	then
	echo "Content type is limited to 14 characters,alphanumeric and \"_\"." > /usr/tmp/err.$VPID
	echo false
else
	/usr/vmsys/admin/PS/CONFIG/alphanum "$1"
	if [ $? = 1 ]
	then
	echo "Content type is limited to 14 characters,alphanumeric and \"_\"." > /usr/tmp/err.$VPID
	echo false
	else
	echo true
	fi
	
fi
