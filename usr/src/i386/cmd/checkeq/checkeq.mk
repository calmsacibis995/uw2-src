#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)checkeq:i386/cmd/checkeq/checkeq.mk	1.1.4.2"
#ident	"$Header: checkeq.mk 1.1 91/05/07 $"

# Makefile for checkeq

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

all: checkeq

checkeq:	checkeq.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

checkeq.o:

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) checkeq

clean:
	rm -f checkeq.o

clobber: clean
	rm -f checkeq
