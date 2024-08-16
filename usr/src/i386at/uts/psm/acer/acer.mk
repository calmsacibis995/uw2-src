#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/acer/acer.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = acer.mk
DIR = psm/acer
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/acer.ln

LOCALINC = -I.

ACER = acer.cf/Driver.o

MODULES = $(ACER)

FILES = \
	acer.o \
	acer_phys.o \
	clockintr.o \
	intr_p.o \
	nmi.o \
	pic.o \
	spl.o \
	aceridb.o \
	acersyspro.o

CFILES = \
	acer.c \
	clockintr.c \
	nmi.c \
	pic.c \
	aceridb.c \
	acersyspro.c 

SFILES = \
	acer_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	acer.ln \
	clockintr.ln \
	nmi.ln \
	pic.ln \
	aceridb.ln \
	acersyspro.ln 

all:	$(MODULES)

install: all
	cd acer.cf; $(IDINSTALL) -R$(CONF) -M acer 

$(ACER): $(FILES)
	$(LD) -r -o $(ACER) $(FILES)

acer_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(ACER)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e acer 

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
