#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisa.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = eisa.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/autoconf/ca/eisa 

EISA = eisa.cf/Driver.o
LFILE = $(LINTDIR)/eisa.ln

MODULES = \
	$(EISA)

FILES = \
	eisanvm.o \
	eisarom.o \
	eisaca.o

CFILES = \
	eisanvm.c \
	eisaca.c

SFILES = \
	eisarom.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	eisanvm.ln \
	eisaca.ln \
	eisarom.ln

all:	$(MODULES)

install: all
	cd eisa.cf; $(IDINSTALL) -R$(CONF) -M eisa

$(EISA): $(FILES)
	$(LD) -r -o $(EISA) $(FILES)


eisarom.o: $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L $(EISA)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e eisa 

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	nvm.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)
 
include $(MAKEFILE).dep

