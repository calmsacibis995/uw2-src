#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)grep:grep.mk	1.6.7.8"

#	Makefile for grep 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = grep.mk

MAINS = grep egrep

OBJECTS =  grep.o

SOURCES =  grep.c

all:	$(MAINS)

grep:	grep.o 
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

egrep:	grep
	-$(RM) -f egrep
	ln grep egrep

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) grep
	-$(CH) $(RM) -f $(INSDIR)/egrep
	$(CH) ln $(INSDIR)/grep $(INSDIR)/egrep

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
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
