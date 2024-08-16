#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/olivetti/olivetti.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = olivetti.mk
DIR = psm/olivetti
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/olivetti.ln

LOCALINC = -I.

OLIVETTI = olivetti.cf/Driver.o

MODULES = $(OLIVETTI)
PROBEFILE = olivetti.c
BINARIES = $(OLIVETTI)

FILES = \
	olivetti.o \
	oli.o \
	oliapic.o \
	apit.o \
	clockintr.o \
	intr_p.o \
	spl.o

CFILES = \
	olivetti.c \
	oliapic.c \
	apit.c \
	clockintr.c

SFILES = \
	oli.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	olivetti.ln \
	oliapic.ln \
	apit.ln \
	clockintr.ln

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi


install: all
	cd olivetti.cf; $(IDINSTALL) -R$(CONF) -M olivetti

binaries : $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $(OLIVETTI) $(FILES)

oli.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e olivetti
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi


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

sysHeaders = \
	olivetti.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
