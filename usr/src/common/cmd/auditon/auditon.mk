#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)auditon:auditon.mk	1.1.13.2"
#ident "$Header: auditon.mk 1.6 91/06/21 $"

include $(CMDRULES)

INSDIR=$(USRSBIN)
DEFAULTDIR=$(ETC)/default
INIT2DIR=$(ETC)/init.d
OWN=root
GRP=audit
SRCDIR=.
LDLIBS=-lia -lcmd

MAKEFILE = auditon.mk
SOURCE = auditon.c
OBJECTS = auditon.o
MAINS = auditon 

all:	$(MAINS)

auditon:	$(OBJECTS)
	$(CC) $(OBJECTS) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditon.o: auditon.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/deflt.h \
	$(INC)/time.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h \
	$(INC)/sys/wait.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) $(MAINS)
	[ -d $(DEFAULTDIR) ] || mkdir -p $(DEFAULTDIR)
	cp audit.dfl audit ; \
	$(INS) -m 444 -u $(OWN) -g sys -f $(DEFAULTDIR) audit ; \
	rm -f audit
	[ -d $(INIT2DIR) ] || mkdir -p $(INIT2DIR)
	cp S02audit audit ; \
	$(INS) -m 444 -u $(OWN) -g $(GRP) -f $(INIT2DIR) audit

strip:
	$(STRIP) $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	[ -d $@ ] || mkdir -p $(INSDIR); \
	$(CH)chmod 755 $(INSDIR); \
	$(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(SRCDIR) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
