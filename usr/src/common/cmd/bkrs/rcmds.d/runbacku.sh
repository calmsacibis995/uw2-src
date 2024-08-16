#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/runbacku.sh	1.2"
#ident	"$Header: $"
TMPFILE=$1
if test -s $TMPFILE
then
	grep "All" $TMPFILE > /dev/null
	if test $? = 0
	then 
		echo "all" > $TMPFILE
	fi
	shift
	/usr/sadm/sysadm/bin/.chkuser -c "/usr/bin/backup $* :`cat $TMPFILE`:" 
else
echo "No users were MARKed for backup.  The MARK function"
echo "key must be used to select users to be backed up."
fi
rm -f $TMPFILE
