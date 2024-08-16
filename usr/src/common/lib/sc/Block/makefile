#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Block/makefile	3.13"

include $(LIBRULES)

PRODUCT = 

OBJECTS = 

all : 

install : all

.c.o : 
	$(C++CMD) -c $(<)

demos:
	cd demos; $(MAKE)

tests:

clean:
	rm -f *.o *.a core a.out
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean

