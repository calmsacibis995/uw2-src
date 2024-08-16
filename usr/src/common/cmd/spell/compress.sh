#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)spell:compress.sh	1.3.1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/spell/compress.sh,v 1.1 91/02/28 20:09:57 ccs Exp $"
#	compress - compress the spell program log

trap 'rm -f /usr/tmp/spellhist;exit' 1 2 3 15
echo "COMPRESSED `date`" > /usr/tmp/spellhist
grep -v ' ' /var/adm/spellhist | sort -fud >> /usr/tmp/spellhist
cp /usr/tmp/spellhist /var/adm
rm -f /usr/tmp/spellhist
