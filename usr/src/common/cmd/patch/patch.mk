#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)patch_p2:patch.mk	1.1"

include $(CMDRULES)

#	Makefile for patch

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen

all: patch

OBJECTS = patch.o inp.o util.o pch.o version.o

patch: patch.o inp.o util.o pch.o version.o
	$(CC) $(CFLAGS) -o patch $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)


install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) patch

clean:
	rm -f patch.o

clobber: clean
	rm -f patch

lintit:
	$(LINT) $(LINTFLAGS) patch.c

