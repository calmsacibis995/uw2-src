#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/sockmod/sockmod.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sockmod.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/sockmod

SOCKMOD = sockmod.cf/Driver.o
LFILE = $(LINTDIR)/sockmod.ln

FILES = \
	sockmod.o

CFILES = \
	sockmod.c

SRCFILES = $(CFILES)

LFILES = \
	sockmod.ln

all: $(SOCKMOD)

install: all
	(cd sockmod.cf; $(IDINSTALL) -R$(CONF) -M sockmod)

$(SOCKMOD): $(FILES)
	$(LD) -r -o $(SOCKMOD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SOCKMOD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sockmod

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
