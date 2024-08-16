#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:G2++/compsrc/makefile	3.6"

include $(LIBRULES)

PRODUCT = g2++comp

OBJECTS = comp.o desc.o typedef.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(C++CMD) -o g2++comp $(OBJECTS) ../g2++lib/libg2++.a $(ARCHIVES)
	$(STRIP) g2++comp

# Compile desc.c w/ inlining turned off to workaround inlining bug
# desc.o : 
# 	$(C++CMD) -Kno_inline -c $(<)

.c.o : 
	$(C++CMD) -c $(<)

tests:

clean:
	$(RM) -f *.ii *.o *.a core a.out

clobber: clean
	$(RM) -f $(PRODUCT)
