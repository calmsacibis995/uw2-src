#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.


#ident	"@(#)chmod:chmod.mk	1.8.5.4"

#	Makefile for chmod 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
INCSYS = $(INC)

#top#
# Generated by makefile 1.47

MAKEFILE = chmod.mk


MAINS = chmod

OBJECTS =  chmod.o

SOURCES =  chmod.c

all:		$(MAINS)

chmod:		chmod.o	
	$(CC) -o chmod  chmod.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

chmod.o: chmod.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/sys/dir.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/ctype.h  \
	$(INC)/unistd.h \
	$(INC)/stdlib.h

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) chmod.c

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

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