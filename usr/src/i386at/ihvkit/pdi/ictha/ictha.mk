#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ihvkit:pdi/ictha/ictha.mk	1.4"

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(LOCALDEF) $(INCLIST) -c -u $*.c >> $*.L

INC = /usr/include
INCLIST = -I$(INC) -I$(INC)/sys
LOCALDEF = -D_KERNEL -DSTATIC=static

GRP = bin
OWN = bin

INS = install
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/ictha.ln
LFILES = ictha.ln

FRC =

HEADERS = ictha.h \
	hba.h

OBJFILES= ictha.o

DRVRFILES = ictha.cf/Driver.o

all:	headinstall $(DRVRFILES) ID
	echo "**** HBA build completed ****" > /dev/null

ID:
	( cd ictha.cf; \
	/etc/conf/bin/idinstall -d ictha > /dev/null 2>&1; \
	/etc/conf/bin/idinstall -a -k ictha; \
	/etc/conf/bin/idbuild -M ictha; \
	cp /etc/conf/mod.d/ictha .; \
	grep ictha /etc/mod_register | sort | uniq >loadmods; \
	cp disk.cfg /etc/conf/pack.d/ictha )
clean:
	rm -f *.o

clobber: clean
	$(IDINSTALL)  -e -d ictha

ictha.cf/Driver.o:	ictha.o
	$(LD) -r -o $@ ictha.o

headinstall:	
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

ictha.o: ictha.c \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/cmn_err.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/scsi.h \
	ictha.h \
	$(INC)/sys/dynstructs.h \
	$(INC)/sys/hba.h \
	$(INC)/sys/kmem.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/dma.h \
	$(INC)/sys/sysenvmt.h \
	$(INC)/sys/moddefs.h \
	$(INC)/sys/ddi.h \
	$(INC)/sys/ddi_i386at.h
	$(CC) $(CFLAGS) $(LOCALDEF) $(INCLIST) -c ictha.c

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done
