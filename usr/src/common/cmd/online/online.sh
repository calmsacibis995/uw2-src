#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/online/online.sh	1.1"
#	NAME:	online
#
#	DESCRIPTION:	take specified processors online
#
#	SYNOPSIS:	online  [-v]  [processor_id ...]
#
#	NOTE: 		This command for compatibility only
#			please use psradm(1m)
#
if [ ${#}  -eq  0 ]	# no arguments
then
	psradm -n
exit
fi
VERBOSE=0
STATE=1
if [ $1 = "-v" ] 	# Check for the verbose flag
then
shift 1
if [ ${#}  -eq  0 ]
then
	psradm -n 
	psrinfo
exit
fi
VERBOSE=1;
fi

for ENG in $* 		# Try to turn on each engine in the list
do
if [ $VERBOSE -eq 1 ]
then
	STATE=`psrinfo -s $ENG`
	if [ $? -ne 0 ]
	then
		exit $? 
	fi
	psrinfo  $ENG
fi

psradm -n  $ENG		# turn on the engine
if [ $? -ne 0 ]		# exit for loop if bad result
then
	exit $? 
fi

if [ $STATE -eq 0 ] 	# Print engine state only if was not already on
	then
		psrinfo $ENG
fi
done
