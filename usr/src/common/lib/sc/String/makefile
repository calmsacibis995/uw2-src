#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:String/makefile	3.12"

include $(LIBRULES)

PRODUCT = ../libString.a

OBJECTS = S_substr.o S_queue.o S_string3.o S_misc.o S_add.o S_int_str.o \
	S_long_str.o S_read.o S_fputs.o S_puts.o S_fgets.o S_gets.o S_sgets.o \
	S_index.o S_input.o S_app.o S_ass.o S_setup.o S_square.o S_put.o \
	S_read1.o S_reserve.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

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
