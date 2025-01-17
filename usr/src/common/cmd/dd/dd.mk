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

#ident	"@(#)dd:dd.mk	1.5.8.4"
#ident "$Header: dd.mk 1.3 91/04/08 $"

#	Makefile for dd 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

#top#
# Generated by makefile 1.47

MAKEFILE = dd.mk

MAINS = dd dd.dy

OBJECTS =  dd.o

SOURCES =  dd.c

all:		$(MAINS)

dd:		dd.o 
	$(CC) -o $@ dd.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

dd.dy:		dd.o 
	$(CC) -o $@ dd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)


dd.o:		 $(INC)/stdio.h $(INC)/signal.h \
		 $(INC)/sys/signal.h $(INC)/sys/param.h \
		 $(INC)/sys/types.h $(INC)/sys/sysmacros.h \
		 $(INC)/sys/stat.h $(INC)/unistd.h $(INC)/stdlib.h \
		 $(INC)/locale.h $(INC)/pfmt.h $(INC)/string.h \
		 $(INC)/errno.h

GLOBALINCS = $(INC)/signal.h $(INC)/stdio.h \
	$(INC)/sys/param.h $(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h $(INC)/sys/types.h \
		 $(INC)/sys/stat.h $(INC)/unistd.h $(INC)/stdlib.h \
		 $(INC)/locale.h $(INC)/pfmt.h $(INC)/string.h \
		 $(INC)/errno.h


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) dd
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) dd.dy
	-/bin/mv $(USRBIN)/dd.dy $(USRBIN)/dd

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
