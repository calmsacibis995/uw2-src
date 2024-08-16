#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mt:mt.sh	1.1"

DEVICE=/dev/rmt/ntape1
TAPECNTL=/usr/bin/tapecntl

while getopts 'f:?' c
do
	case $c in
		f )
			DEVICE=${OPTARG}
			shift;shift
			;;
		\? )
			pfmt -l "mt" -g "uxcore.abi:1275" "Usage: mt [-f tape device ] command [ count ]\n"
			exit 1
			;;
	esac
done

if [ "$#" -lt 1 ]
then
	pfmt -l "mt" -g "uxcore.abi:998" "Invalid command syntax\n"
	pfmt -l "mt" -g "uxcore.abi:1275" "Usage: mt [-f tape device ] command [ count ]\n"
	exit 1
fi

COMMAND=$1

if [ ${2:-0} -lt 0 ]
then
	pfmt -l "mt" -g "uxcore.abi:145" "Bad argument \"%s\"\n" $2
	pfmt -l "mt" -g "uxcore.abi:1275" "Usage: mt [-f tape device ] command [ count ]\n"
	exit 1
fi

case $2 in
	[0-9] | [0-9][0-9] | [0-9][0-9][0-9] )
		COUNT=$2
		;;

	*)
		COUNT=1
		;;
esac

case $COMMAND in
	"rewind" | "rew" )
		$TAPECNTL -w $DEVICE
		if [ $? -gt 0 ]
		then
			exit 2
		fi
		exit 0
		;;

	"fsf")
		$TAPECNTL -p$COUNT $DEVICE
		if [ $? -gt 0 ]
		then
			exit 2
		fi
		exit 0
		;;
	* )
		pfmt -l "mt" -g "uxcore.abi:463" "Unrecognized command: %s\n" $COMMAND
		exit 1
		;;
		
esac



