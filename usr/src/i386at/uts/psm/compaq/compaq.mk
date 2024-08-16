#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/compaq/compaq.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = compaq.mk
DIR = psm/compaq
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/compaq.ln

LOCALINC = -I.

COMPAQ = compaq.cf/Driver.o

MODULES = $(COMPAQ)

FILES = \
	compaq.o \
	compaq_phys.o \
	clockintr.o \
	intr_p.o \
	pic.o \
	spl.o \
	syspro.o \
	xl.o

CFILES = \
	compaq.c \
	clockintr.c \
	pic.c \
	syspro.c \
	xl.c

SFILES = \
	compaq_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	compaq.ln \
	clockintr.ln \
	pic.ln \
	syspro.ln \
	xl.ln

all:	$(MODULES)

install: all
	cd compaq.cf; $(IDINSTALL) -R$(CONF) -M compaq 

$(COMPAQ): $(FILES)
	$(LD) -r -o $(COMPAQ) $(FILES)

compaq_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(COMPAQ)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e compaq 

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
