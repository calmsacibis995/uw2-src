#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pg:pg.mk	1.9.1.4"
#ident "$Header: pg.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lgen -lw -lcurses

all: pg

pg: pg.o
	$(CC) -o pg pg.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pg

clean:
	rm -f pg.o

clobber: clean
	rm -f pg

lintit:
	$(LINT) $(LINTFLAGS) *.c
