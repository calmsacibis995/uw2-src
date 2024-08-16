#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)users:users.mk	1.12.8.4"
#ident  "$Header: users.mk 1.3 91/07/01 $"

include $(CMDRULES)

LDLIBS = -liaf -lgen

OWN = bin
GRP = bin

all: listusers

listusers : users.o 
	$(CC) -o listusers users.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

users.o: users.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/grp.h \
	$(INC)/pwd.h \
	$(INC)/varargs.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/ia.h

install: all 
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) listusers

clean :
	rm -f users.o

clobber : clean
	rm -f listusers

lintit :
	$(LINT) $(LINTFLAGS) users.c
