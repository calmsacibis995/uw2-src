#!/etc/dcu.d/winxksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:common/cmd/initpkg/aconf1_sinit.sh	1.11"

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
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

. /etc/TIMEZONE

if [ "`/sbin/resmgr -k-1 -pRM_INITFILE,s 2>/dev/null`" = "resmgr" ]
then
	if [ /stand/resmgr -nt /stand/resmgr.sav ]
	then
		cp /stand/resmgr /stand/resmgr.sav
	fi
fi

/etc/conf/bin/idconfupdate

DCU_ACTION="`/sbin/resmgr -k-1 -pDCU_ACTION,s 2>/dev/null`"

case "$DCU_ACTION" in

"REBUILD")
	/etc/conf/bin/idbuild 2>&1 > /dev/null
;;
"REBOOT")
	pfmt -l $LABEL -s info -g $CAT:164 \
		"New hardware instance mapped to static driver,\n\
		 rebooting to incorporate addition.\n"

	sleep 5
	/sbin/uadmin 2 1
;;
esac
