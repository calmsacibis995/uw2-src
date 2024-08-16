#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:G2++/g2++lib/makefile	3.7"

include $(LIBRULES)

PRODUCT = libg2++.a

OBJECTS = Text.o g2ctab.o g2ctype.o g2debug.o \
	g2ntostr.o getrec.o manip.o \
	init.o putbuf.o \
	err.o g2tree.o \
	putrec.o g2uio.o \
	getbuf.o \
	g2util.o getdot.o

all : $(PRODUCT)

install : all

.MUTEX: headers objects

$(PRODUCT) : headers objects
	$(AR) $(ARFLAGS) libg2++.a $(OBJECTS)

headers: g2values.h g2ctab.c g2mach.h

objects: $(OBJECTS)

g2values.h: geng2values
	./geng2values
	$(RM) -f ../incl/g2values.h
	$(LN) g2values.h ../incl

g2ctab.c: geng2ctab
	./geng2ctab

g2mach.h: geng2mach
	./geng2mach
	$(RM) -f ../incl/g2mach.h
	$(LN) g2mach.h ../incl

geng2ctab: geng2ctab.c
	$(C++CMD) -o geng2ctab geng2ctab.c $(ARCHIVES)

geng2mach:geng2mach.c
	$(C++CMD) -o geng2mach geng2mach.c $(ARCHIVES)

geng2values: geng2values.c
	$(C++CMD) -o geng2values geng2values.c $(ARCHIVES)

.c.o :
	$(C++CMD) -c $(<) $(ARCHIVES)

clean:
	$(RM) -f libg2++.a geng2mach geng2ctab geng2values
	$(RM) -f g2ctab.c g2mach.h g2values.h ../incl/g2mach.h ../incl/g2values.h
	$(RM) -f *.ii *.o *.a core a.out

clobber: clean
	$(RM) -f $(PRODUCT)
