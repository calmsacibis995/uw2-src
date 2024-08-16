#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libcomplex:libcomplex.mk	1.7"
#
#  makefile for C++ libcomplex.a
#

include $(LIBRULES)

#  This builds both a normal (non-profiled) and a profiled version of
#  the library.  'n' and 'p' respectively serve as directory prefixes
#  and file name suffixes in the building process.

.SUFFIXES: .n .p

LIBP = $(CCSLIB)/libp

PRODUCTS = nobjects/libcomplex.a pobjects/libcomplex.a

.MUTEX: $(PRODUCTS)

SGSBASE = ../../cmd/sgs

DEFLIST=
INCLIST=

CC_CMD = $(C++C) $(CFLAGS) $(DEFLIST) $(INCLIST)

OBJECTS = abs.o arg.o cos.o error.o exp.o io.o log.o oper.o \
          polar.o pow.o sin.o sqrt.o

NOBJECTS = $(OBJECTS:.o=.n)

POBJECTS = $(OBJECTS:.o=.p)

all:	$(PRODUCTS)

install:        all
	if [ ! -d $(LIBP) ];\
	then\
		mkdir $(LIBP);\
	fi
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/libcomplex.a nobjects/libcomplex.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(LIBP)/libcomplex.a pobjects/libcomplex.a

#  the .n and .p files (renamed .o's) persist across makes, while the
#  nobjects and pobjects subdirectories are recreated with each make
 
nobjects/libcomplex.a : $(NOBJECTS)
	$(RM) -rf nobjects
	mkdir nobjects
	for i in $(NOBJECTS);\
	do\
		$(CP) $$i nobjects/`basename $$i .n`.o;\
	done
	cd nobjects; $(AR) q libcomplex.a `lorder $(OBJECTS) | $(TSORT)`

pobjects/libcomplex.a : $(POBJECTS)
	$(RM) -rf pobjects
	mkdir pobjects
	for i in $(POBJECTS);\
	do\
		$(CP) $$i pobjects/`basename $$i .p`.o;\
	done
	cd pobjects; $(AR) q libcomplex.a `lorder $(OBJECTS) | $(TSORT)`

.c.n:
	$(CC_CMD) -c $(?)
	$(MV) $*.o $@

.c.p:
	$(CC_CMD) -c -p $(?)
	$(MV) $*.o $@

clean:
	$(RM) -rf nobjects pobjects $(NOBJECTS) $(POBJECTS)

clobber: clean
	-rm -f $(CCSLIB)/libcomplex.a $(LIBP)/libcomplex.a
