#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)evgainit:evgainit.mk	1.2"

#
# 	evgainit.mk:
# 	makefile for the evgainit command
#

include	$(CMDRULES)

LOCALDEF = -DEVGA -D_LTYPES

all:	evgainit

install:	all
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./evgainit

clean:
	rm -f *.o

clobber: clean
	rm -f evgainit

evgainit.o:	evgainit.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/kd.h \
	$(FRC)
evgainit:	evgainit.o 
	$(CC) -o evgainit evgainit.o $(LDFLAGS) $(ROOTLIBS)
