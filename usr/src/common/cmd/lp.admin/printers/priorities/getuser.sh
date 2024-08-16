#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/priorities/getuser.sh	1.1.4.3"
#ident  "$Header: getuser.sh 2.0 91/07/12 $"
# getuser.c : this replaces awk 
#             if the user list is priorities to remove, then
#		it only lists users that have priority limits.
#
#		If the list is to set priorities, it lists users
#		with login ids over 100.

if [ "$1" = "users" ];
then
	for i in `cat /etc/passwd`
	do
		if [ "`echo $i |fmlcut -f3 -d:`" -ge "100" ]
			then echo $i | fmlcut -f1 -d:;
		fi;
	done
	echo all;
elif [ "$1" = "removep" ];
then
	cat $remove_1;
	echo all;
fi
