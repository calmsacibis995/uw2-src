#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)setclk:i386/cmd/setclk/setclk.mk	1.3.12.1"

include $(CMDRULES)


OWN = bin
GRP = bin

LOCALDEF = -Uu3b -Uvax -Updp11 -Uu3b15 -Uu3b2 -Di386

all: setclk

setclk: setclk.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS)

setclk.o: setclk.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/sys/sysi86.h \
	$(INC)/sys/uadmin.h \
	$(INC)/sys/errno.h \
	$(INC)/time.h

install: all
	-rm -f $(ETC)/setclk
	-rm -f $(USRSBIN)/setclk
	$(INS) -f $(SBIN) -m 0550 -u $(OWN) -g $(GRP) setclk.dy
	$(INS) -f $(SBIN) -m 0550 -u $(OWN) -g $(GRP) setclk
	$(INS) -f $(USRSBIN) -m 0550 -u $(OWN) -g $(GRP) setclk
	-$(SYMLINK) /sbin/setclk $(ETC)/setclk

clean:
	-rm -f setclk.o

clobber: clean
	-rm -f setclk

lintit:
	$(LINT) $(LINTFLAGS) setclk.c
