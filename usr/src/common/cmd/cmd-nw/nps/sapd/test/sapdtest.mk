#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/test/sapdtest.mk	1.4"
#ident	"$Id: sapdtest.mk,v 1.4 1994/03/25 23:16:12 vtag Exp $"

include $(CMDRULES)

include ../../../local.def

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

SRCS = sapdtest.c 

OBJS = sapdtest.o 

LDLIBS = $(LIBUTIL)

all: sapdtest

sapdtest:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDOPTIONS) $(LDLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f sapdtest

