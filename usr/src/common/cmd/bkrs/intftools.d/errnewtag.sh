#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/errnewtag.sh	1.2.6.2"
#ident  "$Header: errnewtag.sh 1.2 91/06/21 $"

# Invoked with tag ($1) and table ($2) and reports either that a null tag
# is invalid or that tag already exists in table.

if [ "$1" ]
then
	echo "Tag already exists in table $2 or has blanks in it."
else
	echo "The tag value cannot be null."
fi
