#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/WHEEL/mkwheel.sh	1.1.1.2"
#ident	"$Header: $"
echo "fault=$2" > /usr/tmp/chgwheel.$VPID
[ "$2" = "mail" ] && echo "login=$LOGNAME" >> /usr/tmp/chgwheel.$VPID
echo "freq=$3" >> /usr/tmp/chgwheel.$VPID
echo "requests=$4" >> /usr/tmp/chgwheel.$VPID

/usr/bin/diff /usr/tmp/wheel.$VPID /usr/tmp/chgwheel.$VPID 2>/dev/null 1>&2

if [ $? = 0 ]
	then echo 1
	/usr/bin/rm -rf /usr/tmp/chgwheel.$VPID
	exit
fi


if [ "$3" = "once" ]
	then freq=0
else
	freq=$3
fi

if [ "$2" = "mail" ] ; then
/usr/lib/lpadmin -S$1 -A"mail $LOGNAME" -W$freq -Q$4  2>/dev/null 1>&2
else
/usr/lib/lpadmin -S$1 -A"$2" -W$freq -Q$4  2>/dev/null 1>&2
fi

/usr/bin/rm -rf /usr/tmp/chgwheel.$VPID

echo 0
