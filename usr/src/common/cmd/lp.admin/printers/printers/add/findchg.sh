#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/findchg.sh	1.1.6.2"
#ident  "$Header: findchg.sh 2.0 91/07/12 $"

#Find character sets for lpadmin

index=0;
for i in $fieldvals;
do
	if grep "cs$index=$i" $saveinit > /dev/null
	then
		:
	else
		echo "cs$index=$i,\c" 
	fi;
	index=`expr $index + 1`
done
echo end;
rm -f $saveinit
