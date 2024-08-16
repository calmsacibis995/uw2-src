#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:IS_pkgsetup.sh	1.5"
###
#
# This script is called by Install_Server.sh which is a wksh
# script that is used to set up an install server.
#
# This script calls the following commands via /sbin/tfadmin
# 	/usr/sbin/pkgcopy
#	/bin/pkgtrans
#	/sbin/mount
#	/sbin/umount
#	kill -- shell builtin
#	/sbin/rm
#	/sbin/ln
# 
# Each of these is executed in a subshell so that they
# retain the Parent PID.  Install_Server.sh calls this
# in the background and watches the execution particularly
# for relatively long-lived processes such as pkgtrans or
# pkgcopy.
#
# NOTE: Packages that have been made available for installation must
# be loaded/linked/mounted in the /var/spool/dist directory.
#
SPOOLAREA=/var/spool/dist
ReturnCode=0
OPTS=""
ADDGO=""
ERRUSAGE=0

#
# Collect arguments and do usage validation.
#
# Usage: IS_pkgsetup pkgcopy|pkgtrans|mount|umount|kill|rm|ln <options>

case "$1" in
    pkgcopy )
	shift
	if [ "$1" = "-v" ]
	then
	    OPTS=$1
	    shift
	fi

	# check for expected usage
	if [ "$1" = "-s" -a "$3" = "-t" ]
	then
	    { /usr/sbin/pkgcopy $OPTS $*; }
	    ReturnCode=$?
	else
	    # unexpected usage encountered
	    echo unexpected usage
	    ERRUSAGE=1
	fi
	;;

    pkgtrans )
	shift
	if [ "$1" = "go" ] 
	then
	    ADDGO=":"
	    shift
	fi

	# check for expected usage
	if [ "$2" = "${SPOOLAREA}" ]
	then
	    if [ "$ADDGO" ]
	    then
		echo "go" | /bin/pkgtrans $* >/dev/null 2>&1
		ReturnCode=$?
	    else
		/bin/pkgtrans $* >/dev/null 2>&1
		ReturnCode=$?
	    fi
	else
	    # unexpected usage encountered
	    echo unexpected usage
	    ERRUSAGE=1
	fi
	;;

    mount )
	shift
	{ /sbin/mount $* 2>/dev/null ; }
	ReturnCode=$?
	;;

    umount )
	shift
	{ /sbin/umount $*; }
	ReturnCode=$?
	;;

    'kill' )
	# kill is a shell built-in
	# make sure this is just the intended pid for the installsrv
	shift
	{ kill $*; }
	ReturnCode=$?
	;;

    rm )
	# make sure we're deleting only from /var/spool/dist
	shift
	{ /sbin/rm $*; }
	ReturnCode=$?
	;;

    ln )
	# make sure we're linking packages to /var/spool/dist
	shift
	{ /sbin/ln $*; }
	ReturnCode=$?
	;;

    mkdir )
	# make sure we're linking packages to /var/spool/dist
	shift
	{ /usr/bin/mkdir $*; }
	ReturnCode=$?
	;;

    * )
	# usage error
	ERRUSAGE=1
	;;

esac

if [ "$ERRUSAGE" -eq 1 ]
then
    ReturnCode=3
fi

# exit with useful return code

exit ${ReturnCode}
