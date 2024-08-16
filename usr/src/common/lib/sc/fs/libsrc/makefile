#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:fs/libsrc/makefile	3.5"

include $(LIBRULES)

# Don't include the instrumented _vec.c, because _vec_new only
# gets called for the cases of new X[...] where X has a constructor.
# In any other case of new ...[...], ::new gets called.
# And _vec_new calls ::new anyway.

OBJECTS=_new.o _delete.o reg.o map.o placenew.o

PRODUCT = libfs.a

all : $(PRODUCT)

$(PRODUCT) : $(OBJECTS)
	$(RM) -f libfs.a
	$(AR) $(ARFLAGS) libfs.a `$(LORDER) $(OBJECTS) | $(TSORT)`

install : all

# reg.c must be compiled with -g to keep debug happy on fs_ calls;
# can't use C++CMD because a -O in it would override -g
reg.o:	reg.c
	$(C++C) $(DEFLIST) $(INCLIST:-I=-I../) $(LINKDIRS:-L=-L../) -c -g reg.c

.c.o : 
	$(C++CMD) -c $(<)

tests:

clean:
	$(RM) -f $(OBJECTS) libfs.a *.o *.ii *.a core a.out

clobber: clean
	$(RM) -f $(PRODUCT)
