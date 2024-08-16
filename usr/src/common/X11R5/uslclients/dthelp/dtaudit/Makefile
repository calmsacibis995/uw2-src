#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dthelp:dtaudit/Makefile	1.1"

all: Tags Refs Match

Tags: Tags.c
	cc -o Tags Tags.c

Refs: Refs.c
	cc -o Refs Refs.c

Match: Match.c
	cc -o Match Match.c -lgen
