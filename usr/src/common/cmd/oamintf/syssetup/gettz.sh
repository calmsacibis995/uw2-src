#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/syssetup/gettz.sh	1.1.5.2"
#ident  "$Header: gettz.sh 2.0 91/07/12 $"

#gettz
# gets proper time zone for screen

tz=`echo $TZ | /usr/bin/cut -c1-4`
case ${tz} in
	'GMT0')	 timez='Greenwich' ;;
	'AST4')	 timez='Atlantic' ;;
	'EST5')	 timez='Eastern' ;;
	'CST6')	 timez='Central' ;;
	'MST7')	 timez='Mountain' ;;
	'PST8')	 timez='Pacific' ;;
	'YST8')	 timez='Yukon' ;;
	'AST1')	 timez='Alaska' ;;
	'BST1')	 timez='Bering' ;;
	'HST1')	 timez='Hawaii' ;;
	'*')	 timez=' ' ;;
esac
echo $timez >/tmp/gettz
