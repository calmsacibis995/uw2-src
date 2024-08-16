#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cksum:cksum.mk	1.1.1.1"

include $(CMDRULES)

#	Makefile for cksum

OWN = bin
GRP = bin

all: cksum

cksum: cksum.c
	$(CC) -o cksum cksum.o cksum.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cksum

clean:
	rm -f cksum.o

clobber: clean
	rm -f cksum

lintit:
	$(LINT) $(LINTFLAGS) cksum.c

#	These targets are useful but optional

partslist:
	@echo cksum.mk cksum.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo cksum | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit cksum.mk cksum.c -o cksum.o cksum
