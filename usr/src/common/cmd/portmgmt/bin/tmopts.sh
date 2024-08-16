#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/tmopts.sh	1.3.7.3"
#ident  "$Header: tmopts.sh 2.0 91/07/13 $"

opts=""

if [ "$1" = no ]
then
	opts="$opts -h"
fi

if [ "$2" = yes ]
then
	opts="$opts -c"
fi

if [ "$3" = yes ]
then
	opts="$opts -b"
fi

if [ "$4" = yes ]
then
	opts="$opts -r $5"
fi

if [ "$6" -gt 0 ]
then
	opts="$opts -t $6"
fi

if [ "$7" != "login: " -a "$7" != login: ]
then
	opts="$opts -p \\\"$7\\\""
fi

if test -n "$8"
then
	opts="$opts -m $8"
fi

if test -n "$9"
then
	opts="$opts -i \\\"$9\\\""
fi

echo -m\"\`\$TFADMIN ttyadm "$opts" \\ 
