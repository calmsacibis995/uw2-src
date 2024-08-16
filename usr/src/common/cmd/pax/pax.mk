#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pax:pax.mk	1.2"

include $(CMDRULES)

#	Makefile for pax

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS)

all: pax

OBJECTS = pax.o append.o buffer.o charmap.o cpio.o \
	create.o extract.o fileio.o \
	hash.o link.o list.o mem.o namelist.o names.o \
	pass.o pathname.o replace.o tar.o \
	ttyio.o warn.o

pax: pax.o append.o buffer.o charmap.o cpio.o \
	create.o extract.o fileio.o \
	hash.o link.o list.o mem.o namelist.o names.o \
	pass.o pathname.o replace.o tar.o \
	ttyio.o warn.o
	$(CC) $(CFLAGS) -o pax $(OBJECTS) $(LDFLAGS) $(LOCAL_LDLIBS) $(ROOTLIBS)


install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pax

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f pax


