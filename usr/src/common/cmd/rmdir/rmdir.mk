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

#ident	"@(#)rmdir:rmdir.mk	1.13.3.2"

include $(CMDRULES)

#	rmdir make file

OWN = bin
GRP = bin

LDLIBS = -lgen

all: rmdir

rmdir: rmdir.o
	$(CC) -o rmdir rmdir.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

rmdir: rmdir.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rmdir

clean:
	-rm -f rmdir.o

clobber: clean
	rm -f rmdir

lintit:
	$(LINT) $(LINTFLAGS) rmdir.c
