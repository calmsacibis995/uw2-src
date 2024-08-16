#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:Path/makefile	3.14"

include $(LIBRULES)

PRODUCT = ../libPath.a

KOBS=test.o expand.o
LOBS=strmatch.o
OBJECTS = $(KOBS) $(LOBS) PATH.o canon.o complete.o ctlist.o die.o dirname.o \
	explode.o find.o is_wd.o kshtest.o makerel.o no_wd.o path.o \
	prefix.o search.o slash.o strdup.o stream1.o \
	stream2.o tilde.o tmppath.o truncate.o wild.o xgetwd.o

all : $(PRODUCT)

install : all

$(PRODUCT) : $(OBJECTS)
	$(AR) $(ARFLAGS) $(@) `$(LORDER) $(OBJECTS) | $(TSORT)`

section3.h:
	sh ./gendefines lstat getwd getcwd >section3.h
 
test.o: ksh/test.c section3.h
	$(CC_CMD) -c ksh/test.c
 
expand.o:ksh/expand.c section3.h
	if test "$(SYS)" = SYSV; then $(CC_CMD) -DFS_3D -c ksh/expand.c; else $(CC_CMD) -c ksh/expand.c; fi
 
strmatch.o:     libx/strmatch.c
	$(CC_CMD) -c libx/strmatch.c
 
.c.o : 
	$(C++CMD) -c $(<)

demos:
	cd demos; $(MAKE)

tests:

clean:
	rm -f *.ii *.o *.a core a.out
	cd ksh; $(MAKE) clean
	cd libx; $(MAKE) clean
	cd demos; $(MAKE) clean SCLIB=../$(SCLIB)

clobber: clean
	rm -f $(PRODUCT)
