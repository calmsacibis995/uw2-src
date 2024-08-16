#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pkgtools:pkg.mk	1.4"

include $(LIBRULES)

#	Makefile for libpkg

LOCALINC =  -Ihdrs

MAKEFILE = libpkg.mk

LIBRARY = libpkg.a

SOURCES = $(OBJECTS:.o=.c)

OBJECTS =  canonize.o ckparam.o ckvolseq.o cvtpath.o devtype.o dstream.o \
	gpkglist.o gpkgmap.o isdir.o logerr.o mappath.o pkgactkey.o \
	pkgexecl.o pkgexecv.o pkgmount.o pkgserid.o pkgtrans.o pkgxpand.o \
	ppkgmap.o privent.o progerr.o putcfile.o rrmdir.o runcmd.o \
	srchcfile.o tputcfent.o verify.o

all:		$(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBRARY)


