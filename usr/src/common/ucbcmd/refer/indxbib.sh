#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/refer/indxbib.sh	1.2"
#ident	"$Header: $"

#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved

#
#	indxbib sh script
#
if test $1
	then /usr/ucblib/reftools/mkey $* | /usr/ucblib/reftools/inv _$1
	mv _$1.ia $1.ia
	mv _$1.ib $1.ib
	mv _$1.ic $1.ic
else
	echo 'Usage:  indxbib database [ ... ]
	first argument is the basename for indexes
	indexes will be called database.{ia,ib,ic}'
fi
