#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libthread:common/lib/libthread/tsd/tsd.mk	1.5"

# 
# libthread: thread-specific data
#

MAKEFILE = tsd.mk
LIBTHREADRULES = ../thread/../libthread.rules

CSRCS = tsd.c
COBJS = tsd.$O

include $(LIBRULES)
include $(LIBTHREADRULES)

# Dependencies

tsd.o: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/trace.h \
	$(INC)/errno.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/memory.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/synch.h \
	$(INC)/synonyms.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/list.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/regset.h \
	$(INC)/sys/select.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysmacros_f.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/usync.h \
	$(INC)/thread.h \
	$(INC)/time.h \
	$(INC)/ucontext.h

