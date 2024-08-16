#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:Regex/makefile	3.15"

include $(LIBRULES)

PRODUCT = ../libRegex.a

OBJECTS = Regex.o Rematch.o Reiter.o Subex.o recomp.o reexec.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

#recomp.c is in C
recomp.o :
	$(CC_CMD) -c $(<) -I./libx libx/recomp.c

reerror.o :
	$(C++CMD) -c $(<) -I./libx libx/reerror.c

#reexec.c is in C
reexec.o :
	$(CC_CMD) -c $(<) -I./libx libx/reexec.c

resub.o :
	$(C++CMD) -c $(<) -I./libx libx/resub.c

t.o :
	$(C++CMD) -c $(<) -I./libx libx/t.c

.c.o : 
	$(C++CMD) -c $(<)

demos:
	cd demos; $(MAKE)

tests:

clean:
	rm -f *.ii *.o *.a core a.out
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	rm -f $(PRODUCT)
