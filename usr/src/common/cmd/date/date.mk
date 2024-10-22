#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)date:common/cmd/date/date.mk	1.1.14.4"

#	Makefile for date 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

#top#
# Generated by makefile 1.47

MAKEFILE = date.mk

MAINS = date

OBJECTS =  date.o

SOURCES =  date.c

all:		$(MAINS)

date:		date.o 
	$(CC) -o $@ date.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

date.o: \
	$(INC)/errno.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/stdarg.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/time.h \
	$(INC)/unistd.h \
	$(INC)/utmp.h \
	$(INC)/utmpx.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/select.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/unistd.h

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
