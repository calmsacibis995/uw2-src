#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Args/makefile	3.13"

include $(LIBRULES)

PRODUCT = ../libArgs.a

OBJECTS = Args.o Argsarg.o Argsflag.o Argsiter.o Argsobj.o Argsopt.o \
	Argsset.o Argssub.o Argsvalue.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

$(OBJECTS): Argslib.h incl/Args.h

.c.o : 
	$(C++CMD) -c $(<)

demos:
	cd demos; $(MAKE)

tests:

clean:
	rm -f *.o *.a core a.out
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	rm -f $(PRODUCT)
