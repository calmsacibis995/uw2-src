#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmas:libmas.mk	1.1"

#	Makefile for libmas

include $(LIBRULES)

LDFLAGS =
LFLAGS = -G -dy -ztext
HFLAGS = -h /usr/lib/libmas.so

LOCALDEF = $(PICFLAG) 
LINTFLAGS = 

MAKEFILE = libmas.mk

LIBRARY = libmas.a
DOTSO = libmas.so

OBJECTS =  mas_consume.o mas_error.o mas_provide.o mas_types.o 

all:
	$(MAKE) -f libmas.mk clean $(LIBRARY) PICFLAG='' $(MAKEARGS)
	$(MAKE) -f libmas.mk clean $(DOTSO)  PICFLAG='$(PICFLAG)' $(MAKEARGS)
		

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(OBJECTS) | $(TSORT)`

$(DOTSO):	$(OBJECTS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(OBJECTS) $(LIBRARY) $(DOTSO) 

install: all
	rm -f $(USRLIB)/$(LIBRARY)
	rm -f $(USRLIB)/$(DOTSO)
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)

lintit:	
	$(LINT) $(LINTFLAGS) *.c
	
