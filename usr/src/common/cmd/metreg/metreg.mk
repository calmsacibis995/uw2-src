#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)metreg:metreg.mk	1.5"


include $(CMDRULES)

#	Makefile for metreg

LOCALDEF = -D_KMEMUSER
OWN = bin
GRP = bin
INSDIR = $(SBIN)
SYMLINK = :
LDLIBS = -lmas
MAKEFILE = metreg.mk
MAINS = metreg

OBJECTS = metreg.o
SOURCES = metreg.c

all:	$(MAINS) 

metreg:	metreg.o 
	$(CC) -o metreg metreg.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)



metreg.o:	$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/unistd.h \
		$(INC)/fcntl.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/metrics.h \
		$(INC)/sys/plocal.h \
		$(INC)/sys/metdisk.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/mman.h \
		$(INC)/mas.h \
		$(INC)/metreg.h \
		mettbl.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install: all
	 $(INS) -f  $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

lintit:

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
