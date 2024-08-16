#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pathchk:pathchk.mk	1.1.1.1"

include $(CMDRULES)

#	Makefile for pathchk

OWN = bin
GRP = bin

all: pathchk

pathchk: pathchk.o
	$(CC) -o pathchk pathchk.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pathchk.o: pathchk.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pathchk

clean:
	rm -f pathchk.o

clobber: clean
	rm -f pathchk

lintit:
	$(LINT) $(LINTFLAGS) pathchk.c

#	These targets are useful but optional

partslist:
	@echo pathchk.mk pathchk.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo pathchk | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit pathchk.mk $(LOCALINCS) pathchk.c -o pathchk.o pathchk
