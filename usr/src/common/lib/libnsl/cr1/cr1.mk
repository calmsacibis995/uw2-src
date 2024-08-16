#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/cr1/cr1.mk	1.1.8.2"
#ident	"$Header: $"

include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o

# 
# Network Services Library: Challange/Response Scheme #1 routines
#
LOCALDEF=-DNO_IMPORT $(NSL_LOCALDEF)
LOCALINC=-I.
CR1INCLUDES=	\
		cr1.h \
		cr1_mt.h \
		$(INC)/cr1.h 

SRCS=		getkey.c \
		cr1_mt.c

INCLUDES=	\
		cr1.h \
		cr1_mt.h \
		$(INC)/cr1.h \
		$(INC)/crypt.h \
		$(INC)/fcntl.h \
		$(INC)/pwd.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/string.h \
		$(INC)/unistd.h \
		$(INC)/rpc/types.h \
		$(INC)/rpc/xdr.h \
		$(INC)/sys/types.h

LIBOBJS = getkey.o cr1_mt.o

OBJS =	../getkey.o ../cr1_mt.o

all:       $(INCLUDES) $(LIBOBJS)
	cp $(LIBOBJS) ..

install:

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) getkey.c

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OBJS)
