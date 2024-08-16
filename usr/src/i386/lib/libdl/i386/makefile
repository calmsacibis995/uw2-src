#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libdl:i386/makefile	1.5"
#
# i386-specific makefile for libdl
#
#

include $(LIBRULES)

SGSBASE=../../../cmd/sgs
INS=$(SGSBASE)/sgs.install
INSDIR=
HFILES=
SOURCES= $(COMDIR)/libdl.c
OBJECTS= libdl.o
PRODUCTS=
DEFLIST=
COMDIR= ../common
LDFLAGS= -G -h /usr/lib/libdl.so.1

#
all: libdl.so
	mv libdl.so ../libdl.so

libdl.so:	$(OBJECTS)
	$(CC) -o libdl.so $(LDFLAGS) $(OBJECTS)

libdl.o:	$(COMDIR)/libdl.c
	$(CC) -c $(CFLAGS) $(MODE) $(INCLIST) $(COMDIR)/libdl.c 

install:	all
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/libdl.so ../libdl.so

clean:
	rm -f *.o

clobber:	clean
	rm -f libdl.so ../libdl.so
