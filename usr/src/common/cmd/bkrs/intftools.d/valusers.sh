#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/valusers.sh	1.2.6.2"
#ident  "$Header: valusers.sh 1.2 91/06/21 $"
# validate that list of users is in password file
USERS="$1"
USERS=`echo $USERS | sed -e "s/,/ /g"`
for i in $USERS
do
	grep "^${i}:" /etc/passwd >/dev/null
	if [ $? -ne 0 ]
	then
		exit 1
	fi
done
exit 0
