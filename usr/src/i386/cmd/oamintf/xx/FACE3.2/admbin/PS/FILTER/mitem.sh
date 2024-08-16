#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/mitem.sh	1.1.1.2"
#ident	"$Header: $"
if echo "$1" | /usr/bin/grep any > /dev/null
then	echo Y
	exit
else	
	a=`echo "$1" | /usr/bin/tr "," " "`
	if [ "$2" = type ]
	then
		/usr/bin/cut -f1 /usr/vmsys/admin/PS/PORTSET/database > /usr/tmp/type$$
		sfile=/usr/tmp/type$$
	else	sfile=/usr/tmp/pname.$3
	fi
	set -- "$a"
	for i in $*
	do
		if grep "^$i$" $sfile > /dev/null
		then	:
		else	echo $i
			exit
		fi
	done
	echo Y
fi
/usr/bin/rm -f /usr/tmp/type$$
