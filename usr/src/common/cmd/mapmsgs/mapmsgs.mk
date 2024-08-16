#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mapmsgs:mapmsgs.mk	1.1"

include $(CMDRULES)

LDLIBS = -lgen

all: mapmsgs

mapmsgs: mapmsgs.o
	$(CC) -o mapmsgs mapmsgs.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	strip mapmsgs

mapmsgs.o: mapmsgs.c mapmsgs.h

install: all
	$(INS) -f $(USRBIN) -m 0555 mapmsgs

clean:
	rm -f *.o

clobber: clean
	rm -f mapmsgs

lintit:
	$(LINT) $(LINTFLAGS) mapmsgs.c

#       These targets are useful but optional

partslist:
	@echo mapmsgs.mk mapmsgs.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo mapmsgs | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mapmsgs.mk $(LOCALINCS) mapmsgs.c -o mapmsgs.o mapmsgs



