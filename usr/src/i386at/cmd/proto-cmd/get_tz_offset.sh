#!/usr/bin/winxksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/get_tz_offset.sh	1.1"

struct tm tm_sec tm_min tm_hour tm_mday tm_mon tm_year tm_wday tm_yday tm_isdst
cdecl 'int *' altzone='&altzone' timezone='&timezone'
call time
cdecl 'int *' time=$_RETX
call -c localtime time
cdecl tm local_tm=p$_RETX
cprint -v isdst local_tm.tm_isdst
if (( isdst ))
then
	cprint altzone
else
	cprint timezone
fi
