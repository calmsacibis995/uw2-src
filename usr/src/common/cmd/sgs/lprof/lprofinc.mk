#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:lprofinc.mk	1.8"

PROF_SAVE	=
SRCBASE		= common


INS		= $(SGSBASE)/sgs.install
INSDIR		= $(CCSBIN)
HFILES		= 
SOURCES		=
OBJECTS		=
PRODUCTS	=

PLBBASE		= $(SGSBASE)/lprof/libprof
LIBPROF		= $(SGSBASE)/lprof/libprof/$(CPU)/libprof.a
LIBSYMINT	= $(SGSBASE)/lprof/libprof/$(CPU)/libsymint.a
LDFLAGS		= -s
