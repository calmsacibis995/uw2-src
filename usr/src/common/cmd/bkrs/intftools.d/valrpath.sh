#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/valrpath.sh	1.1.6.2"
#ident  "$Header: valrpath.sh 1.2 91/06/21 $"
# script to validate path name for a restore job.
# A null path is legal (restores to original object's path).
# A file name is only legal if the type of restore is "file", otherwise
# a directory is required.  The directory will be created if it doesn't
# already exist.
# Script must be called with $1 = restore type, $2 = target path.
TYPE="$1"
TARGET=`echo "$2" | sed -e "s/  *//"`

if [ "$2" = "" ]
then
	exit 0
fi
if [ "$1" = "file" ]
then
	valpath -aw $2
else
	valpath -aytw $2
fi
exit $?
