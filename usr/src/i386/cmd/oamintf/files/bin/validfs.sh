#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/validfs.sh	1.1"
#ident	"$Header: $"

# Argument is not a directory

if [ ! -d ${1} ]
then
	exit 1
fi

# Mount point is busy

if [ ! -z "`/sbin/mount | sed -e 's/ .*//' | grep ${1}`" ]
then
	exit 2
fi

# Directory has files in it

if [ "`echo \`ls -la ${1} | wc -l\``" != "3" ]
then
	exit 3
fi
