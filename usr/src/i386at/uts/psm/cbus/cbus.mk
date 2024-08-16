#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/cbus/cbus.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = cbus.mk
DIR = psm/cbus
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/cbus.ln

LOCALINC = -I.

CBUS = cbus.cf/Driver.o

MODULES = $(CBUS)

FILES = \
	cbus.o \
	cbus_phys.o \
	cbus1.o \
	cbus2.o \
	cbusapic.o \
	clockintr.o \
	corollary.o \
	intr_p.o \
	pic.o \
	spl.o

CFILES = \
	cbus.c \
	cbus1.c \
	cbus2.c \
	cbusapic.c \
	clockintr.c \
	corollary.c \
	pic.c

SFILES = \
	cbus_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	cbus.ln \
	cbus1.ln \
	cbus2.ln \
	cbusapic.ln \
	clockintr.ln \
	corollary.ln \
	pic.ln

all:	$(MODULES)

install: all
	cd cbus.cf; $(IDINSTALL) -R$(CONF) -M cbus 

$(CBUS): $(FILES)
	$(LD) -r -o $(CBUS) $(FILES)

cbus_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(CBUS)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e cbus 

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
