#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	 All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wc:wc.mk	1.6.4.2"

include $(CMDRULES)

#	Makefile for wc 

OWN = bin
GRP = bin

all: wc

wc: wc.o 
	$(CC) -o wc wc.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

wc.o:	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/stdlib.h

install: all
	 $(INS) -f $(USRBIN) -m 00555 -u $(OWN) -g $(GRP) wc

clean:
	rm -f wc.o

clobber: clean
	rm -f wc

lintit:
	$(LINT) $(LINTFLAGS) wc.c

#	These targets are useful but optional

partslist:
	@echo wc.mk wc.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo wc | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit wc.mk $(LOCALINCS) wc.c -o wc.o wc
