#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)csplit:csplit.mk	1.4.3.2"
#ident	"$Header: csplit.mk 1.3 91/04/08 $"

#	Makefile for csplit

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS = -lgen

MAKEFILE = csplit.mk

MAINS = csplit

OBJECTS =  csplit.o

SOURCES =  csplit.c

all:		$(MAINS)

csplit:	$(SOURCES)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $(SOURCES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install: all
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)
