#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/chkmrgfiles.sh	1.13"
#ident	"$Header: $"

### main ()
#  script to display the screen:
#  
#  Do you want to auto merge config files or not
#
#  The arg is the name of the pkg
#  This script is invoked from  the request script of a pkg

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

NAME="$1"

Set_LANG
Chk_Color_Console
export TERM

#
#  Now invoke the menu program with everything we just extracted.
#

unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany && set +x

menu -f ${UPGRADE_MSGS}/mergefiles.3 -o /tmp/response.$$ </dev/tty

[ "$UPDEBUG" = "YES" ] && set -x

. /tmp/response.$$
rm -f /tmp/response.$$
	
#	RETURN_VALUE=1 for Yes. Merge files
#	RETURN_VALUE=2 for No. Do not auto merge files.

rc=`expr $RETURN_VALUE - 1`
unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany

exit  $rc
