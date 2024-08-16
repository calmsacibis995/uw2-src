#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libcmd/i386/makefile	1.3"

include $(CMDRULES)

TARGET = $(PRODLIB)/libcmd.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Execute.C \
	$(COMMON)/Help.C \
	$(COMMON)/Input.C \
	$(COMMON)/Keyword.C \
	$(COMMON)/Location.C \
	$(COMMON)/Parser.C \
	$(COMMON)/Scanner.C \
	$(COMMON)/Shell.C \
	$(COMMON)/systable.C \
	sysmach.C

CSOURCES = \
	Help_msg.c \
	Help_thr.c

include DEPEND

include ../../util/common/lib.make
