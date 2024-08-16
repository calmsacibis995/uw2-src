#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/pcmp/pcmp.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = pcmp.mk
DIR = psm/pcmp
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/pcmp.ln

LOCALINC = -I.

PCMP = pcmp.cf/Driver.o

MODULES = $(PCMP)

FILES = \
	pcmp.o \
	pcmp_phys.o \
	apic.o \
	apit.o \
	clockintr.o \
	intr_p.o \
	spl.o

CFILES = \
	pcmp.c \
	apic.c \
	apit.c \
	clockintr.c

SFILES = \
	pcmp_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	pcmp.ln \
	apic.ln \
	apit.ln \
	clockintr.ln

all:	$(MODULES)

install: all
	cd pcmp.cf; $(IDINSTALL) -R$(CONF) -M pcmp 

$(PCMP): $(FILES)
	$(LD) -r -o $(PCMP) $(FILES)

pcmp_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(PCMP)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e pcmp 

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
