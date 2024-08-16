#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/online/offline.sh	1.1"
#	NAME:	offline
#
#	DESCRIPTION:	take specified processors offline
#
#	SYNOPSIS:	offline  [-v]  [processor_id ...]
#
#	NOTE: 		This command for compatibility only
#			please use psradm(1m)

if [ ${#}  -eq  0 ]
then
	psradm -f -a
exit
fi
VERBOSE=0
STATE=0

if [ $1 = "-v" ] 	# Check for the verbose flag
then
shift 1
if [ ${#}  -eq  0 ]	# just "offline -v"
then
	psradm -f -a 
	psrinfo
exit
fi
VERBOSE=1;
fi

for ENG in $* 		# Try to turn off each engine in the list
do
if [ $VERBOSE -eq 1 ]
then
	STATE=`psrinfo -s $ENG`	
	psrinfo  $ENG
fi

psradm -f  $ENG		# The actual offline
if [ $STATE -eq 1 ] 	# Print engine state only if was not already offline
	then
		psrinfo $ENG
fi
done
