#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:i386/makefile	1.5"

include $(CMDRULES)

LPROFBASE	= `pwd | sed 's/lprof.*/lprof/'`

SGSBASE		= $(LPROFBASE)/..
CMDBASE		= $(LPROFBASE)/../..

include ../lprofinc.mk

all install lintit clean clobber:
	cd ..; $(MAKE) -f lprof.mk $@
