#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

 
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/Makefile	1.2"

include	$(LIBRULES)

#
# Define shared library revision.
#
REV=1

#
# List include directories 
#
INCLUDE=-I$(ROOT)/$(MACH)/usr/X/include -I.

#
# Define compiler options: -K PIC is for shared objects.
#
CCOPTIONS= -c -K PIC

#
# Define optimizer options
#
OPTIMIZE=-O

#
# Define debugging options
#
DEBUG=
   
#
# Define loader options: -G is for shared library.
#
LDOPTIONS= -G -h libolim.so.$(REV)


#
# List sources that make up the IM library.
#

SRCS=OlCloseIm.c \
     OlCreateIc.c \
     OlDestroyIc.c \
     OlDpyOfIm.c \
     OlGetIcV.c \
     OlGetImV.c \
     OlImOfIc.c \
     OlImUtil.c \
     OlLcOfIm.c \
     OlLookupImS.c \
     OlOpenIm.c \
     OlResetIc.c \
     OlSetIcF.c \
     OlSetIcV.c \
     OlUnsetIcF.c

OBJS=OlCloseIm.o \
     OlCreateIc.o \
     OlDestroyIc.o \
     OlDpyOfIm.o \
     OlGetIcV.o \
     OlGetImV.o \
     OlImOfIc.o \
     OlImUtil.o \
     OlLcOfIm.o \
     OlLookupImS.o \
     OlOpenIm.o \
     OlResetIc.o \
     OlSetIcF.o \
     OlSetIcV.o \
     OlUnsetIcF.o

libolim: oldlib $(SRCS) $(OBJS)
	@$(LD) $(LDOPTIONS) -o libolim.so.$(REV) $(OBJS)

clean:
	@-rm -f *.o

.c.o:   
	@$(CC) $(CCOPTIONS) $(OPTIMIZE) $(DEBUG) $(INCLUDE) $*.c

oldlib:	
	@-rm -f libolim.so.$(REV)
