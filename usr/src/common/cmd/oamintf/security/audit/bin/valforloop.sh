#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/security/audit/bin/valforloop.sh	1.1.3.2"
#ident  "$Header: valforloop.sh 2.0 91/07/12 $"

# simple program to get around FMLI inability to do for loops
# $1 should be all items entered in FMLI field
# $2 should be what we're testing them for
for i in $1
do
	if eval "$2" ; then
		continue
	else
		exit 1
	fi
done

