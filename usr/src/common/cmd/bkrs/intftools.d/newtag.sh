#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/newtag.sh	1.3.6.2"
#ident  "$Header: newtag.sh 1.2 91/06/21 $"

# Validate that tag ($1) does not already exist in table ($2).
# Note that a null tag is not valid.
# Any nonnull tag is valid for a nonexistent table (new), but no tag
# may have a blank in it.

blanks=`expr "$1" : '.*\(  *\).*'`
if [ -n "$blanks" ]
then
	exit 1
elif [ -s "$2" ]
then
	if [ -z "$1" ]
	then
		exit 1
	else
# tag_exists returns true if tag exists
		tag_exists $1 $2
		if [ $? -ne 0 ]
		then
			exit 0
		else
			exit 1
		fi
	fi
else
	exit 0
fi
