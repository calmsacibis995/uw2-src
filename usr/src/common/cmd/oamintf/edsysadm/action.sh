#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/action.sh	1.1"
#ident  "$Header: action.sh 2.0 91/07/12 $"

action=$INTFBASE/main.menu
path=`dirname $action`
rest=`echo "$1:" |cut -d':' -f'2-'`

name=`echo $rest | cut -d':' -f1`
rest=`echo $rest | cut -d':' -f'2-'`

while [ ."$name" != ."" ]
do
	actionfile=`grep "^$name\^" ${action} | cut -d'^' -f3`
	case $actionfile in
	/*)	action=$actionfile;;
	*)	action=$path/$actionfile;;
	esac
	path=`dirname $action`
	name=`echo $rest | cut -d':' -f1`
	rest=`echo $rest | cut -d':' -f'2-'`
done
echo $action
