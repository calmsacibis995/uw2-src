#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/rstab.sh	1.1.10.3"
#ident "$Header: rstab.sh 1.2 91/04/26 $"

#	place share(1M) commands here for automatic execution
#	on entering init state 3.
#
#	share [-F fstype] [ -o options] [-d "<description>"] <pathname>
#	.e.g,
#	share -F nfs -d "/var/news" /var/news
