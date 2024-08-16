#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)who:common/cmd/who/who.mk	1.9.5.7"
#ident "$Header: who.mk 1.3 91/04/29 $"

include $(CMDRULES)

#	Makefile for who 

OWN = bin
GRP = bin

MAINS = who who.dy
OBJECTS = who.o Getinittab.o
LDLIBS = -lgen

all: $(MAINS)

who: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

who.dy: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

who.o:  $(INC)/errno.h $(INC)/sys/errno.h \
	 $(INC)/fcntl.h $(INC)/stdio.h \
	 $(INC)/string.h $(INC)/sys/types.h \
	 $(INC)/unistd.h $(INC)/stdlib.h \
	 $(INC)/sys/stat.h \
	 $(INC)/pfmt.h

Getinittab.o:  $(INC)/errno.h $(INC)/sys/errno.h \
	 $(INC)/fcntl.h $(INC)/stdio.h \
	 $(INC)/string.h $(INC)/sys/types.h \
	 $(INC)/unistd.h $(INC)/stdlib.h \
	 $(INC)/sys/stat.h $(INC)/time.h \
	 $(INC)/utmp.h $(INC)/locale.h \
	 $(INC)/pfmt.h

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) who
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) who.dy
	-/bin/mv $(USRBIN)/who.dy $(USRBIN)/who

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) who.c

# These targets are useful but optional

partslist:
	@echo who.mk who.c c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo who | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit who.mk $(LOCALINCS) who.c -o who.o who
