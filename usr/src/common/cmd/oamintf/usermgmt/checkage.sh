#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/checkage.sh	1.1.4.2"
#ident  "$Header: checkage.sh 2.0 91/07/12 $"

############################################################################
#	Command: checkage
#
#	Description: 	This script acquires the next availabe uid
#			and then checks it against all not sufficiently
#			aged uids listed in /etc/security/ia/ageduid.
#			If the uid retrieved is not sufficiently aged,
#			we bump it up and check this new one against
#			the contents of the file repeatedly until a uid
#			that is sufficiently aged is found.
############################################################################

USERID=`finduid`
while [ true ]
do
	AGED=`$TFADMIN cat /etc/security/ia/ageduid  | grep "^$USERID:" | cut -f1 -d":"`
	if [ "$AGED" = "$USERID" ]
	then
		USERID=`expr $USERID + 1`
		continue
	else
		break
	fi
done
echo $USERID
