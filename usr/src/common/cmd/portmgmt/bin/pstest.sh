#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/pstest.sh	1.2.6.2"
#ident  "$Header: pstest.sh 2.0 91/07/13 $"

# pstest - check to see if there is at least one service 
#	   to enable, disable or remove.
# Input:   $1 - type of operation, (i.e. enable, disable, or remove)

OK=0		# at least one item is found
NOTHING=1	# no item is found

case $1 in
	enable)
		psfile=`pmadm -L|cut -d: -f4|grep "x"| wc -l`;;
	disable)
		psfile=`pmadm -L|cut -d: -f4|grep -v "x"| wc -l`;;
	remove)
		psfile=`pmadm -L| wc -l`;;
	*)
		exit $NOTHING;;
esac

if [ $psfile = 0 ]
then
	exit $NOTHING
else
	exit $OK
fi
