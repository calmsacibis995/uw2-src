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

#ident	"@(#)cmp:cmp.mk	1.6.4.2"

#	Makefile for cmp 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

#top#
# Generated by makefile 1.47

MAKEFILE = cmp.mk

MAINS = cmp

OBJECTS =  cmp.o

SOURCES =  cmp.c

all:		$(MAINS)

cmp:		cmp.o 
	$(CC) -o cmp  cmp.o   $(LDFLAGS) $(LDLIBS) $(PERFLIBS)


cmp.o:		 $(INC)/stdio.h $(INC)/ctype.h $(INC)/priv.h \
		 $(INC)/locale.h $(INC)/pfmt.h \
		 $(INC)/errno.h $(INC)/string.h

GLOBALINCS = $(INC)/ctype.h $(INC)/stdio.h $(INC)/priv.h \
	$(INC)/locale.h $(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/string.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) cmp

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
