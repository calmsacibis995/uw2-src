#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/vfstab.sh	1.1.7.2"
echo "#special          fsckdev          mountp   fstype fsckpass automnt mntopts
/dev/root	/dev/root	/	s5	1	yes	-
">vfstab
