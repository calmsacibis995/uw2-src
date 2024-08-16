#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:incl/makefile	3.10"

include $(LIBRULES)

PRODUCT = incl2

OBJECTS = incl2.o

all : $(PRODUCT)

$(PRODUCT) : $(OBJECTS)
	$(C++CMD) -o incl2 $(OBJECTS) $(ARCHIVES)
	chmod +x bin/incl

incl2.o : incl2.c
	$(C++CMD) -c incl2.c

install: all

clean:
	$(RM) -f *.o *.ii *.a core a.out

clobber: clean
	$(RM) -f $(PRODUCT)

