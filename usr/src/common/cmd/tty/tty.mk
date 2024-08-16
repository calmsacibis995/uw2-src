#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)tty:tty.mk	1.4.3.2"

include $(CMDRULES)

#	tty make file

OWN = bin
GRP = bin

LDLIBS = -lm

all: tty

tty: tty.c \
	$(INC)/stdio.h \
	$(INC)/sys/stermio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tty

clean:
	rm -f tty.o

clobber: clean
	 rm -f tty
	
lintit:
	$(LINT) $(LINTFLAGS) tty.c
