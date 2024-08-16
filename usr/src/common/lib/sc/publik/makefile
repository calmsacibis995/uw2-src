#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:publik/makefile	3.8"

include $(LIBRULES)

PRODUCT = publik2

OBJECTS=CXXLexer.o CXXToken.o Lexer.o Token.o parse.o publik2.o scope.o

all : $(PRODUCT)
	chmod +x bin/publik

$(PRODUCT):	$(OBJECTS)
	$(C++CMD) $(CCFLAGS) -o publik2 $(OBJECTS) $(ARCHIVES)

.c.o :
	$(C++CMD) -c $(<) $(ARCHIVES)

install: all

clean:	
	$(RM) -f *.o *.ii core a.out

clobber:clean
	$(RM) -rf $(PRODUCT)
