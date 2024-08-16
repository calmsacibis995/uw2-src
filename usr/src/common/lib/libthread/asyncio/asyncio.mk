#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libthread:common/lib/libthread/asyncio/asyncio.mk	1.2.3.7"

#
# libthread: async i/o
#

MAKEFILE = asyncio.mk
LIBTHREADRULES = ../thread/../libthread.rules

CSRCS = asyncio.c
COBJS = asyncio.$O

include $(LIBRULES)
include $(LIBTHREADRULES)

# Dependencies

asyncio.$O: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/synonyms.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/synch.h \
	$(COMMINC)/thread.h \
	./aiosys.h \
	$(INC)/aio.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/setjmp.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/aiosys.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/list.h \
	$(INC)/sys/listasm.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/proc.h \
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
	$(INC)/sys/unistd.h \
	$(INC)/sys/usync.h \
	$(INC)/time.h \
	$(INC)/ucontext.h \
	$(INC)/unistd.h

