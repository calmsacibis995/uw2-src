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

#ident	"@(#)exstr:exstr.mk	1.3.5.1"
#ident "$Header: exstr.mk 1.2 91/04/08 $"

#	Makefile for exstr

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = exstr.mk

MAINS = exstr

OBJECTS =  exstr.o

SOURCES =  exstr.c

all:		$(MAINS)

exstr:		exstr.o 
	$(CC) -o $@ exstr.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)


exstr.o:		 $(INC)/stdio.h $(INC)/sys/types.h \
		 $(INC)/sys/stat.h 

GLOBALINCS = $(INC)/stdio.h $(INC)/vargs.h $(INC)/string.h \
	$(INC)/sys/signal.h 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
