#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/call_fstyp.sh	1.1"
#ident	"$Header: $"
#!/sbin/sh
#
/sbin/fstyp $1 >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo 0
else
	echo 1
fi
