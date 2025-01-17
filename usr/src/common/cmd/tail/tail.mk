#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)tail:tail.mk	1.6.9.3"

include $(CMDRULES)

#	Makefile for tail

OWN = bin
GRP = bin

all: tail

tail: tail.o
	$(CC) -o tail tail.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tail.o: tail.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/stdlib.h

clean:
	rm -f tail.o

clobber: clean
	rm -f tail

lintit:
	$(LINT) $(LINTFLAGS) tail.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tail 

#	These targets are useful but optional

partslist:
	@echo tail.mk tail.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo tail | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit tail.mk $(LOCALINCS) tail.c -o tail.o tail
