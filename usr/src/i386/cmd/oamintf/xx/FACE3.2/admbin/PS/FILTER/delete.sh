#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/delete.sh	1.1.1.2"
#ident	"$Header: $"
if [ "$1" = all ]
then	
#	/usr/lib/lpfilter -fall -x
	/usr/bin/cut -d: -f5 /usr/spool/lp/admins/lp/filter.table > /usr/tmp/base$$
	for i in `/usr/bin/cat /usr/tmp/base$$`
	do
		case $i in
		435_table | 475_table | 455_table | 473_table | HP_table )
			;;
	* )
		/usr/lib/lpfilter -f $i -x
			;;
		esac
	done
	/usr/bin/rm -f /usr/tmp/base$$
else
	list=`echo "$1" | /usr/bin/tr "," " "`
	set -- "$list"
	for i in $*
	do
		/usr/lib/lpfilter -f $i -x
	done
fi
