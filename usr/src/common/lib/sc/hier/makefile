#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident  "@(#)sc:hier/makefile	3.9"

include $(LIBRULES)

PRODUCT = hier2

OBJECTS = hier2.o parse.o CXXLexer.o CXXToken.o Lexer.o Token.o

all:    $(PRODUCT)
	chmod +x bin/hier

$(PRODUCT):	$(OBJECTS)
	$(C++CMD) -o $@ $(OBJECTS) $(ARCHIVES)

.c.o :
	$(C++CMD) -c $(<) $(ARCHIVES)

install: all

clean: 
	$(RM) -f *.dag *.dvi *.ps *.o *.ii core a.out

clobber:clean
	$(RM) -f $(PRODUCT)

