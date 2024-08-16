#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:G2++/demos/makefile	1.4"

include $(LIBRULES)
include $(SCLIB)/demorules

g2++comp=../../$(LSCBIN)/g2++comp

# demo directory must be cleaned out first,
# otherwise get instantiation problems

all: 	clean test1.o usr.o
	$(SHELL) $(SCBIN)/regress

test1.c: test1.g
	$(RM) -f test1.h test1.c
	$(g2++comp) test1.g

test1.o:	test1.c
	$(C++CMD) -c test1.c

usr.c: usr.g
	rm -f usr.h usr.c
	$(g2++comp) usr.g

usr.o:	usr.c
	$(C++CMD) -c usr.c

.c.r:
	$(C++CMD) -o $*.E $< usr.o test1.o -lg2++ $(ARCHIVES)
	./$*.E <usr.data > $*.r

clean:	
	$(RM) -f test1.c test1.h
	$(RM) -f usr.c usr.h
	$(CLEAN_DEMOS)

clobber: clean
