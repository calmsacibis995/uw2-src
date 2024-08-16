#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/bin/getname.sh	1.1.1.2"
#ident	"$Header: $"
if [ -s /etc/.devices/$1 ]
then
	. /etc/.devices/$1
	if [ "$TYPE" = 'Printer' ]
	then echo $NAME
	else echo ""
	fi
else	echo ""
fi
