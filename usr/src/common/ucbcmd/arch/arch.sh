#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/arch/arch.sh	1.2"
#ident	"$Header: $"
#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


# This shell script offers compatibility with the SunOS
# command arch. It invokes the uname routine with the -m
# option to retreive the same information.

uname -m
