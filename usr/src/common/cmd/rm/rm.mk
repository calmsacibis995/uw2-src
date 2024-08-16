#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rm:rm.mk	1.5.7.5"

include $(CMDRULES)

#	Makefile for rm

OWN = bin
GRP = bin

MAINS = rm rm.dy

all: $(MAINS)

rm: rm.o
	$(CC) -o $@ rm.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

rm.dy: rm.o
	$(CC) -o $@ rm.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rm.o: rm.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/nl_types.h \
	$(INC)/langinfo.h

install: all
	 $(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) rm
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rm.dy
	 -/bin/mv $(USRBIN)/rm.dy $(USRBIN)/rm

clean:
	rm -f rm.o

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) rm.c

#	These targets are useful but optional

partslist:
	@echo rm.mk rm.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo rm | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit rm.mk $(LOCALINCS) rm.c -o rm.o rm rm.dy
