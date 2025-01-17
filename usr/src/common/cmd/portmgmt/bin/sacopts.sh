#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/sacopts.sh	1.3.6.3"
#ident  "$Header: sacopts.sh 2.0 91/07/13 $"

# sacopts - form a string containing sac options

opts=""
if [ "$1" = no ]
then
	opts="$opts -fx"
fi

if [ "$2" = DISABLED ]
then
	opts="$opts -fd"
fi

if [ "$3" -gt 0 ]
then
	opts="$opts -n $3"
fi

if test -n "$4"
then
	opts="$opts -z $4"
fi

if test -n "$5"
then
	opts="$opts -y \"$5\""
fi

echo "$opts" 
