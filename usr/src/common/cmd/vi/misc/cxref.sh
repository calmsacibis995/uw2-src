#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.




#ident	"@(#)vi:misc/cxref.sh	1.4.1.4"
#ident  "$Header: cxref.sh 1.2 91/06/27 $"
grep -n "^[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]" $* > /tmp/$$
ex - /tmp/$$ <<\!
v/(.*)$/d
g/STATIC/d
g/\<static\>/d
g/\<long\>/d
g/\<short\>/d
g/\<line\>/d
g/\<switch\>/d
g/\<unsigned\>/d
g/\<return\>/d
g/\<break\>/d
g/\<bool\>/d
g/\<boolean\>/d
g/\<case\>/d
g/\<struct\>/d
g/\<int\>/d
g/\<char\>/d
g/\<extern\>/d
g/:$/d
g/\\/d
1,$s/\(.*:\)\(.*\)/\2|\1/
1,$s/|/                                                 /
1,$s/^\(................................................\) */\1/
w
q
!
sort /tmp/$$
rm /tmp/$$
