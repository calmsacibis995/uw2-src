#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Path/libx/makefile	3.5"
# These routines are taken from libx.  You can find
# the entire libx library in the ksh or nmake source.
#
include $(LIBRULES)

PRODUCT = libx.a

OBJECTS = strmatch.o

all : $(PRODUCT)

install : all


$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

libx.a:	$(OBJECTS)
	$(RM) -f $@
	$(AR) rc $@ $(OBJECTS)

.c.o : 
	$(CC_CMD) -c $(<)

t:	t.c
	$(CC_CMD) -o t t.c libx.a

tests:	

clean:
	$(RM) -f *.o *.a core a.out

clobber: clean
	$(RM) -f $(PRODUCT)

