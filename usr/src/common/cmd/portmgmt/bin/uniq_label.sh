#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/uniq_label.sh	1.1.6.2"
#ident  "$Header: uniq_label.sh 2.0 91/07/13 $"

# uniq_label - check if the ttylabel selected is uniq

NOTHING=1	# Nothing entered
NOTUNIQ=2	# Not unique
OK=0		

# Nothing entered
test -z "$1" && exit $NOTHING

if grep "^$1[ ]*:" /etc/ttydefs > /dev/null 2>&1
then
	exit $NOTUNIQ
else
	exit $OK
fi
