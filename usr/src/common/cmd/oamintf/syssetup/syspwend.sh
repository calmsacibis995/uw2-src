#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/syssetup/syspwend.sh	1.1.5.2"
#ident  "$Header: syspwend.sh 2.0 91/07/12 $"
#syspwend

syslogs=`/usr/bin/sed -n '/^[^:]*:[^:]*:.\{1,2\}:/s/:.*//p' /etc/passwd | /usr/bin/sort -u`

# get system logins which do not have a password
	for sys in $syslogs
	do
		if /usr/bin/passwd -s $sys 2>>/tmp/sysend | /usr/bin/grep "^$sys  *LK" >/dev/null
		then
			echo "   $sys" >> /tmp/sysend
		fi
	done

