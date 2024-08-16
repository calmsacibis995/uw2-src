#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)langsup:i386/ls/cmd/loadfont/loadfont.sh	1.2"

eval `defadm locale LANG 2> /dev/null`
eval `defadm coterm MBCONSOLE 2> /dev/null`

if [ $? = 0 -a "$MBCONSOLE" = "yes" ]
then
	if [ -x /sbin/pcfont -a ! -z "$LANG" ]
	then
		/sbin/pcfont -l $LANG
	fi
else
	eval `defadm cofont COFONT 2> /dev/null`
	if [ $? = 0  -a ! -z "$COFONT" -a -x /sbin/pcfont ]
	then
		/sbin/pcfont $COFONT
	fi
fi
