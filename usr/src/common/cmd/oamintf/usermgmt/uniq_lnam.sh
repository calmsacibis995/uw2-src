#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/uniq_lnam.sh	1.2.7.2"
#ident  "$Header: uniq_lnam.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: uniq_lnam
#
# 	This functions is used for checking if login ID is unique.
#	 
#	Grep for 'login ID' in /etc/passwd, it must be at the
#	beginning of the line.  If the name is found exit 2
#	(false - since the name is already taken), exit 0 if
#	the name is not found (true - the name chosen is unique).
#
#	$1 - User input
#	$2 - Field default (only in usermod)
################################################################################

NOTHING=1	# Nothing entered
NOTUNIQ=2	# Not unique
OK=0		# login name (ID) is valid
# Nothing entered
test -z "$1" && exit $NOTHING

# Choose default
test "$1" = "$2" && exit $OK

if /usr/bin/grep "^$1:" /etc/passwd > /dev/null 2>&1
then
	exit $NOTUNIQ
else
	exit $OK
fi
