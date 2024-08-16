#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)head:head.mk	1.2.5.2"

#     Makefile for head

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

#top#

MAKEFILE = head.mk

MAINS = head 

OBJECTS =  head.o

SOURCES = head.c 

all:          $(MAINS)

$(MAINS):	head.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	
head.o:		$(INC)/stdio.h 

GLOBALINCS = $(INC)/stdio.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install:	all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) head 

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#     These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)

