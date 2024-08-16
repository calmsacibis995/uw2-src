#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/acct.sh	1.3.7.7"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	export LANG
fi
LABEL="UX:$0"
CAT=uxrc

state=$1
set `LC_ALL=C who -r`
if [ $8 != "0" ]
then
	exit
fi
case $state in

'start')
	if [ $9 = "2" -o $9 = "3" ]
	then
		exit
	fi
	pfmt -l $LABEL -s info -g $CAT:1 "Starting process accounting\n"
	/usr/lib/acct/startup
	;;
'stop')
	pfmt -l $LABEL -s info -g $CAT:2 "Stopping process accounting\n"
	/usr/lib/acct/shutacct
	;;
esac
