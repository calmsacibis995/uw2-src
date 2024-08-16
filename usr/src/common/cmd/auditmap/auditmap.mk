#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)auditmap:auditmap.mk	1.14.2.3"
#ident  "$Header: auditmap.mk 1.4 91/06/21 $"

include $(CMDRULES)

INSDIR=$(USRSBIN)
OWN=root
GRP=audit
SRCDIR=.
LOCALDEF=-D_KMEMUSER
LDLIBS=-lia -lcmd -lmalloc -liaf -lgen

MAKEFILE = auditmap.mk
SOURCE = auditltdb.c auditmap.c
OBJECTS = auditltdb.o auditmap.o 
MAINS = auditmap

all:	$(MAINS)

auditmap:	$(OBJECTS)
	$(CC) $(OBJECTS) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

auditltdb.o: auditltdb.c \
	$(INC)/sys/types.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h \
	$(INC)/audit.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/stdlib.h \
	$(INC)/sys/uio.h \
	$(INC)/unistd.h \
	auditmap.h

auditmap.o: auditmap.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/sys/time.h \
	$(INC)/string.h \
	$(INC)/mac.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/resource.h \
	$(INC)/audit.h \
	$(INC)/sys/auditrec.h \
	$(INC)/ia.h \
	$(INC)/grp.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	auditmap.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) $(MAINS)

strip:
	$(STRIP) $(MAINS)

lintit:
	$(LINT) $(CFLAGS) $(SOURCE)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 755 $@ ;\
		$(CH)chown bin $@

partslist:
	@echo $(MAKEFILE) $(SRCDIR) $(SOURCE)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
