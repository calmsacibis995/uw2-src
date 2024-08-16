#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sa:common/cmd/sa/perf.sh	1.4.6.7"

if [ -n "$_AUTOBOOT" ]
then
	mldmode > /dev/null 2>&1
	if [ "$?" = "0" ]
	then
		exec su sys -c "/sbin/tfadmin /usr/lib/sa/sadc /var/adm/sa/sa`date +%d`"
	else
		exec su sys -c "/usr/lib/sa/sadc /var/adm/sa/sa`date +%d`"
	fi
fi
