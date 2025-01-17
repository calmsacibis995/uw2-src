#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ca:ca/ca.mk	1.2"

include $(CMDRULES)

MAKEFILE = ca.mk

BINS = ca

all:	$(BINS)

clean:
	rm -f *.o ca 


ca: ca.o
	$(CC) -O -s -o $@ ca.o

ca.o: ca.c \
	$(INC)/sys/ca.h \
	$(INC)/sys/nvm.h

install: $(BINS)
	$(INS) -f $(SBIN) -m 0555 -u bin -g bin ca
