#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/isoam.read.sh	1.1.3.1"
#ident	"$Header: $"

# Post a message if an attempt to read in a preSVR4 package is made

echo "
	You may not spool a preSVR4 package. The read_in function is
	applicable only to 4.0 style packages.
"

exit 0
