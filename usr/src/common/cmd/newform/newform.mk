#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)newform:newform.mk	1.2.7.1"
#ident "$Header: newform.mk 1.2 91/04/16 $"

include $(CMDRULES)

#	Makefile for newform

OWN = bin
GRP = bin

LDLIBS = -lw

all: newform

newform: newform.o
	$(CC) -o newform newform.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

newform.o: newform.c \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) newform

clean:
	rm -f newform.o

clobber: clean
	rm -f newform

lintit:
	$(LINT) $(LINTFLAGS) newform.c

