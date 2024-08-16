#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:fs/fsippsrc/makefile	3.6"

include $(LIBRULES)

PRODUCT = fsipp

OBJECTS=CXXToken.o Lexer.o CXXLexer.o fsipp.o parse.o scope.o

all:	$(PRODUCT)

install : all

$(PRODUCT):	$(OBJECTS)
	$(C++CMD) -o fsipp $(OBJECTS) $(ARCHIVES)

.c.o : 
	$(C++CMD) -c $(<)

tests:

clean:
	$(RM) -f *.o *.a *.ii core a.out

clobber: clean
	$(RM) -f $(PRODUCT)
