#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	copyright	"%c"	

#ident	"@(#)join:join.mk	1.3.5.2"

include $(CMDRULES)

#	Makefile for join

OWN = bin
GRP = bin

LDLIBS = -lw

all: join

join: join.o
	$(CC) -o join join.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

join.o: join.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/stdlib.h $(INC)/limits.h $(INC)/ctype.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) join

clean:
	rm -f join.o

clobber: clean
	rm -f join

lintit:
	$(LINT) $(LINTFLAGS) join.c
