#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/atup/atup.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = atup.mk
DIR = psm/atup
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/atup.ln

ATUP = atup.cf/Driver.o

MODULES = $(ATUP)

FILES = \
	atup.o \
	atup_phys.o \
	clockintr.o \
	intr_p.o \
	pic.o \
	spl.o

CFILES = \
	atup.c \
	clockintr.c \
	pic.c

SFILES = \
	atup_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	atup.ln \
	clockintr.ln \
	pic.ln

all:	$(MODULES)

install: all
	cd atup.cf; $(IDINSTALL) -R$(CONF) -M atup 

$(ATUP): $(FILES)
	$(LD) -r -o $(ATUP) $(FILES)

atup_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(ATUP)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e atup 

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
