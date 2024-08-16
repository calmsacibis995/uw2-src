#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)swap:swap.mk	1.12.3.6"
#ident	"$Header: $"

include $(CMDRULES)

OWN = bin
GRP = sys

LDLIBS = $(LIBELF)

SOURCES = swap.c

all: swap swap.dy

swap: swap.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

swap.dy: swap.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

install: all
	-rm -f $(ETC)/swap $(USRSBIN)/swap
	 $(INS) -f $(SBIN) -m 02755 -u $(OWN) -g $(GRP) swap
	 $(INS) -f $(SBIN) -m 02755 -u $(OWN) -g $(GRP) swap.dy
	-$(SYMLINK) /sbin/swap $(ETC)/swap
	-$(SYMLINK) /sbin/swap $(USRSBIN)/swap

clean:
	-rm -f swap.o

clobber: clean
	-rm -f swap swap.dy
	
lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
