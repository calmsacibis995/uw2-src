#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/rmwhite.sh	1.2"

function rmwhite
{
	(( $# == 2 )) || {
		print -u2 "ERROR:USAGE: $0 infile outfile"
		return 1
	}

	# this script removes all leading white space, and then
	# removes all blank lines and lines starting with "#", except if the
	# "#" is followed by a "!" as in "#!/usr/bin/ksh"

	checkwhite < $1
	case $? in
	0)
		print -u2 "$0: filtering whitespace from $1"
		sed -e 's/^[ 	]*//' -e '/^#[^!]/D' -e '/^$/D' -e '/^#$/D' $1 > $2
		;;
	1)
		print -u2 "$0: WARNING: here doc in $1: filtering whitespace anyway"
		sed -e 's/^[ 	]*//' -e '/^#[^!]/D' -e '/^$/D' -e '/^#$/D' $1 > $2
		;;
	2)
		print -u2 "$0: NOT filtering whitespace from $1"
		cp $1 $2
		;;
	esac
	return 0
}
