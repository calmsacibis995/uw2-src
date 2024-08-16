#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/uniq_uid.sh	1.2.7.3"
#ident  "$Header: uniq_uid.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: uniq_uid
#
# 	This functions is used for checking 3 things: 1) input was entered
#	2) if default is input 3) if user group ID is unique.
#
#	$1 - User input
#	$2 - Field default (only in usermod)
################################################################################
NOTHING=1	# Nothing entered
NOTUNIQ=2	# Not unique
OK=0		# login name (ID) is valid

# Field is mandatory
test -z "$1" && exit $NOTHING

# Is field same as default?
test "$1" = "$2" && exit $OK

# Is uid unique
if /usr/bin/grep "^[^:]*:[^:]*:$1:" /etc/passwd > /dev/null 2>&1
then
	exit $NOTUNIQ
else
	exit $OK
fi
