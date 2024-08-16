#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/nullzero/nullzero.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	nullzero.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/nullzero

NULLZERO = nullzero.cf/Driver.o
LFILE = $(LINTDIR)/nullzero.ln

FILES = \
	nullzero.o

CFILES = \
	nullzero.c

SRCFILES = $(CFILES)

LFILES = \
	nullzero.ln

all:	$(NULLZERO)

install: all
	(cd nullzero.cf; $(IDINSTALL) -R$(CONF) -M nullzero)
	

$(NULLZERO):	$(FILES)
	$(LD) -r -o $(NULLZERO) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(NULLZERO)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e nullzero

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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
