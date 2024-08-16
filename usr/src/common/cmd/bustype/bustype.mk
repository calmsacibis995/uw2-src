#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1993 UNIVEL

#ident	"@(#)bustype:bustype.mk	1.1"
#ident "$Header: $"

include $(CMDRULES)


OWN = bin
GRP = bin

all: bustype

bustype: bustype.o
	$(CC) -o bustype bustype.o $(LDFLAGS)

bustype.o: bustype.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/sysi86.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) bustype

clean:
	-rm -f bustype.o

clobber: clean
	rm -f bustype

lintit:
	$(LINT) $(LINTFLAGS) *.c
