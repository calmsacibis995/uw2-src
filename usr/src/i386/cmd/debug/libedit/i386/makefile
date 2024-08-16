#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libedit/i386/makefile	1.1"

include $(CMDRULES)

TARGET = $(PRODLIB)/libedit.a

include ../../util/common/std.make

CSOURCES = \
	../common/edit.c \
	../common/editlib.c \
	../common/emacs.c \
	../common/history.c \
	../common/vi.c \
	../common/defs.c

CCSOURCES = \
	../common/ksh_fc.C \
	../common/debug_read.C

include DEPEND

include ../../util/common/lib.make
