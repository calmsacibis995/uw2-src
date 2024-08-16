#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pdi.cmds:diskaddrm.sh	1.3"

# This script provides the switch-out mechanism for calling the proper diskadd
# or the diskrm command according to the disk management type.

label=UX:`basename $0`
msgdb=uxdiskaddrm

. /etc/default/dskmgmt
dmtype=$DMDEFAULT
while getopts 'F:' c
do
	case $c in
		F)	dmtype=$OPTARG;;
		\?)	pfmt -l $label -s action -g $msgdb:1 "usage: %s [-F DMType] [cCCbBtTTdDD]\n" "$0"
			exit 2;;
		*)	pfmt -l $label -s error -g $msgdb:2 "Internal error\n"
			exit 3;;
	esac
done
if [ -z "$dmtype" ]
then
	dmtype="s5dm"
fi
cmd=`basename $0`
shift `expr $OPTIND - 1`
/etc/diskmgmt/$dmtype/$cmd $*
exit 0
