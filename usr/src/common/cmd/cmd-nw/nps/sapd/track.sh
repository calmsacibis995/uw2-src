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

#ident	"$Id: track.sh,v 1.3 1994/05/18 18:45:21 vtag Exp $"
#
#
# Copyright 1991, 1992 Novell, Inc.
# All Rights Reserved.
#
# This work is subject to U.S. and International copyright laws and
# treaties.  No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged,
# condensed, expanded, collected, compiled, linked, recast,
# transformed or adapted without the prior written consent
# of Novell.  Any use or exploitation of this work without
# authorization could subject the perpetrator to criminal and
# civil liability.
#

#
#   Make sure we are root
#
ID="`id | cut -d'=' -f2 | cut -d'(' -f1`"
if [ "$ID" -ne 0 ]
then
	echo "You must be a root user to run track"
	exit 1
fi

program=`basename $0`

configdir=`nwcm -C | sed -e 's/^.*=//' -e 's/"//g'`
sapdpid=`cat ${configdir}/sapd.pid 2>/dev/null`
if [ "${sapdpid}" -eq 0 ]
then
	echo "${program}: sapd is not running"
	exit 1
fi

case "$1" in
on|ON)
	kill -USR1 ${sapdpid}
	;;
off|OFF)
	kill -USR2 ${sapdpid}
	;;
tables|TABLES)
	kill -PIPE ${sapdpid}
	;;
*)
	echo "Usage: ${program} {on|off|tables}\n"
	exit 1
	;;
esac
exit 0
