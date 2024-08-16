#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/chkstty.sh	1.1.6.2"
#ident  "$Header: chkstty.sh 2.0 91/07/12 $"
	rm -f $error;
	sttyvals=`echo "$1" | tr "," " "` 
	for i in $sttyvals
	do
		stty $i > /dev/null 2> $error;
		if [ -s $error ];
		then
			echo false;
			exit;
		fi
	done 
echo true