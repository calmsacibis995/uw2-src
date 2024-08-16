#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libint/i386/makefile	1.2"

include $(CMDRULES)

TARGET = $(PRODLIB)/libint.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Interface.C \
	$(COMMON)/SrcFile.C \
	$(COMMON)/Output.C \
	$(COMMON)/Manager.C \
	$(COMMON)/GManager.C \
	$(COMMON)/Msgtab.C \
	$(COMMON)/Message.C \
	$(COMMON)/Mformat.C \
	$(COMMON)/Transport.C \
	$(COMMON)/UIutil.C \
	print.C \
	Msgtypes.C \
	Unbundle.C

CSOURCES=\
	Mtable.c

include DEPEND

include ../../util/common/lib.make
