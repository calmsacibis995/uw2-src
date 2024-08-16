#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)resmgr:resmgr.mk	1.2"

include	$(CMDRULES)

MAINS= resmgr resmgr.dy
LDLIBS= -lgen -lresmgr

all:	$(MAINS)

install:	all 
	$(INS) -f $(SBIN) resmgr
	$(INS) -f $(SBIN) resmgr.dy

clean:
	rm -f *.o

clobber: clean
	rm -f $(MAINS)

resmgr:	resmgr.o
	$(CC) -o resmgr resmgr.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

resmgr.dy:	resmgr.o
	$(CC) -o resmgr.dy resmgr.o $(LDFLAGS) $(LDLIBS)
