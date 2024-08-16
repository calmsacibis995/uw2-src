#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)liby:liby.mk	1.13"

include $(LIBRULES)

SGSBASE=../../cmd/sgs
INS=$(SGSBASE)/sgs.install
STRIP=strip

SOURCES=libmai.c libzer.c
OBJECTS=libmai.o libzer.o

all:     $(CCSLIB)/liby.a

$(CCSLIB)/liby.a: $(OBJECTS)
	$(AR) $(ARFLAGS) tmplib.a `$(LORDER) *.o | $(TSORT)`;

libmai.o:	libmai.c
		$(CC) -c $(CFLAGS) libmai.c

libzer.o:	libzer.c
		$(CC) -c $(CFLAGS) libzer.c

install:  all
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/liby.a tmplib.a;

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:clean
	-rm -f tmplib.a
