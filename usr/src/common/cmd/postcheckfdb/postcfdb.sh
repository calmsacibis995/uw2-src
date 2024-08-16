#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)postcheckfdb:postcfdb.sh	1.1"

[ -d /usr/X/lib/locale ] || exit 0
[ -d /usr/X/lib/classdb ] || exit 0
[ -f /usr/X/lib/classdb/dtadmin ] || exit 0
grep "^INCLUDE" /usr/X/lib/classdb/dtadmin >/tmp/i.list.$$
cd /usr/X/lib/locale
for i in [a-z][a-z]
do
	[ -d $i ] || continue
	[ $i = C ] && continue
	[ -d $i/classdb ] || continue
	[ -f $i/classdb/dtadmin ] || continue
	ed $i/classdb/dtadmin <<! 2>/dev/null >/dev/null
g/^INCLUDE/d
r /tmp/i.list.$$
w
q
!
done
rm /tmp/i.list.$$
exit 0
