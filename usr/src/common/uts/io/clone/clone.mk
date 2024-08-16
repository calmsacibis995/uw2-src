#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/clone/clone.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	clone.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/clone

CLONE = clone.cf/Driver.o
LFILE = $(LINTDIR)/clone.ln

FILES = \
	clone.o

CFILES = \
	clone.c

SRCFILES = $(CFILES)

LFILES = \
	clone.ln


all: $(CLONE)

install: all
	(cd clone.cf; $(IDINSTALL) -R$(CONF) -M clone)

$(CLONE): $(FILES)
	$(LD) -r -o $(CLONE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CLONE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e clone

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
