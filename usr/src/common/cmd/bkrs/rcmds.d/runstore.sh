#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/runstore.sh	1.1"
#ident	"$Header: $"
FLPINFO=$1
TMPFILE=$2
FILES=
if test -s $TMPFILE
then
	for i in `cat $TMPFILE`
	do
		if grep "^D	$i$" $FLPINFO > /dev/null
		then
			FILES="$FILES :$i/*:" 
		else
			FILES="$FILES :$i:"
		fi
	done
	shift 2
	/usr/sadm/sysadm/bin/.chkuser -c "/usr/bin/restore $* $FILES"
else
	echo "No files or directories were MARKed for restore."
	echo "The MARK function key must be used to select files"
	echo "or directories to be restored."
fi
rm -f $TMPFILE
rm -f $FLPINFO
