#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)auditlog:auditlog.mk	1.1.11.2"
#ident  "$Header: auditlog.mk 1.4 91/07/01 $"

include $(CMDRULES)

OWN=root
GRP=audit

INSDIR=$(USRSBIN)
LDLIBS=-lia -lcmd

MAKEFILE = auditlog.mk
SOURCE = auditlog.c
OBJECTS = auditlog.o
MAINS = auditlog

all:	$(MAINS)

auditlog:	$(OBJECTS)
	$(CC) $(OBJECTS) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditlog.o: auditlog.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/deflt.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) $(MAINS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

strip:
	$(STRIP) $(MAINS)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	mkdir $(INSDIR);  $(CH)chmod 755 $(INSDIR);  $(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(LOCALINC) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
