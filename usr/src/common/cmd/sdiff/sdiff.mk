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

#ident	"@(#)sdiff:sdiff.mk	1.2.6.1"
#ident "$Header: sdiff.mk 1.2 91/03/20 $"

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS = -lw

all: sdiff

sdiff: sdiff.o 
	$(CC) -o sdiff sdiff.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

sdiff.o: sdiff.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h

install: sdiff 
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sdiff

clean:
	rm -f sdiff.o
	
clobber: clean
	rm -f sdiff

lintit:
	$(LINT) $(LINTFLAGS) sdiff.c

# optional targets

remove:
	cd $(USRBIN); rm -f sdiff

partslist:
	@echo sdiff.mk $(LOCALINCS) sdiff.c | tr ' ' '\012' | sort

product:
	@echo sdiff | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)
