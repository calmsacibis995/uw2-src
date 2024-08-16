#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnsl:common/lib/libnsl/cs/cs.mk	1.1.8.4"
#ident	"$Header: $"

include $(LIBRULES)
include ../libnsl.rules
INCLUDES=$(INC)/cs.h

.SUFFIXES:	.c .o

# Makefile for the libnsl connection server interface 

LOCALDEF=	-DNO_IMPORT $(NSL_LOCALDEF)
#LOCALDEF=	-DDEBUG_ON -DNO_IMPORT $(NSL_LOCALDEF)

OBJS = 	csi.o 

SRCS = $(OBJS:.o=.c)

all:		$(OBJS)
		cp $(OBJS) ..
lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS)

clean:
	rm -f $(OBJS)

clobber:	clean
