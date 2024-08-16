#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/mt/mt.mk	1.2.2.2"
#ident	"$Header: $"

# 
# Makefile for multithread support portion of user level networking library

include $(LIBRULES)
include ../libnsl.rules

INCLUDES=	$(INC)/mt.h

.SUFFIXES:	.c .o

LOCALDEF =	$(NSL_LOCALDEF)

OBJS =		mt.o 

LIBOBJS=	../mt.o 

SRCS = $(OBJS:.o=.c)

all: $(OBJS)
	cp $(OBJS) ../

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIBOBJS)

strip:	all
	$(STRIP) $(LIBOBJS)

size:	all
	$(SIZE) $(LIBOBJS)
