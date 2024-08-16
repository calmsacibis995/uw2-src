#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:Bits/makefile	3.13"

include $(LIBRULES)

PRODUCT = ../libBits.a

OBJECTS = Bits.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

.c.o : 
	$(C++CMD) -c $(<)

tests:

clean:
	rm -f *.ii *.o *.a core a.out
#	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	rm -f $(PRODUCT)
