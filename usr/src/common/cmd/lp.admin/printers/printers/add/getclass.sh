#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/getclass.sh	1.3.9.3"
#ident  "$Header: getclass.sh 2.0 91/07/12 $"
# getclass.sh : Get only the classes linked to the printer
# to use for default values.

if [ "$lp_default" = "none" ]
then
	echo none;
	exit;
fi
if grep "^$lp_default$" /etc/lp/classes/* > /tmp/class.$$ 2> /dev/null;
then
	if [ "`ls /etc/lp/classes/* | wc -l`" = "      1 " ]
	then
		ls /etc/lp/classes;
	else
		cat /tmp/class.$$ | sed 's/\/etc\/lp\/classes\///gp' |
        	cut -f1 -d":" | tail -1f;
	fi
else
	echo none;
fi;
rm -f /tmp/class.$$
