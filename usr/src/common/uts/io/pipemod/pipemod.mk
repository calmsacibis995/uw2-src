#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/pipemod/pipemod.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	pipemod.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/pipemod

PIPEMOD = pipemod.cf/Driver.o
LFILE = $(LINTDIR)/pipemod.ln

FILES = \
	pipemod.o

CFILES = \
	pipemod.c

SRCFILES = $(CFILES)

LFILES = \
	pipemod.ln


all: $(PIPEMOD)

install: all
	(cd pipemod.cf; $(IDINSTALL) -R$(CONF) -M pipemod)

$(PIPEMOD): $(FILES)
	$(LD) -r -o $(PIPEMOD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PIPEMOD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e pipemod

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
