#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/WHEEL/rmwheel.sh	1.1.1.2"
#ident	"$Header: $"


wheels=`echo $1 | /usr/bin/sed 's/,/ /g'`
if [ "$wheels" = "all" ]
	then 
	for wheel in `ls /usr/spool/lp/admins/lp/pwheels`
	do
	/usr/lib/lpadmin -S $wheel -A none 2>/dev/null 1>&2
	done
#if the user select few printwheels 
else
	for wheel in $wheels
	do
	/usr/lib/lpadmin -S $wheel -A none 2>/dev/null 1>&2
	done

fi
