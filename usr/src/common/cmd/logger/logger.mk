#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)logger:logger.mk	1.1.1.1"

include $(CMDRULES)

#	Makefile for logger

OWN = bin
GRP = bin

all: logger

logger: logger.o
	$(CC) -o logger logger.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

logger.o: logger.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) logger

clean:
	rm -f logger.o

clobber: clean
	rm -f logger

lintit:
	$(LINT) $(LINTFLAGS) logger.c

#	These targets are useful but optional

partslist:
	@echo logger.mk logger.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo logger | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit logger.mk $(LOCALINCS) logger.c -o logger.o logger
