#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)wchrtbl:wchrtbl.mk	1.2.2.3"

include $(CMDRULES)

#	Makefile for wchrtbl

all: wchrtbl

wchrtbl: wchrtbl.o 
	$(CC) -o wchrtbl wchrtbl.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

wchrtbl.o: $(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/widec.h \
	$(INC)/wctype.h \
	$(INC)/varargs.h \
	$(INC)/string.h \
	$(INC)/signal.h

install: all
	$(INS) -f $(USRBIN) wchrtbl

clean:
	rm -f wchrtbl.o

clobber: clean
	rm -f wchrtbl

lintit:
	$(LINT) $(LINTFLAGS) wchrtbl.c

# These targets are useful but optional

partslist:
	@echo wchrtbl.mk wchrtbl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo wchrtbl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit wchrtbl.mk $(LOCALINCS) wchrtbl.c -o wchrtbl.o wchrtbl
