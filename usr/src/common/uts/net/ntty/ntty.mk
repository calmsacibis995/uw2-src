#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/ntty/ntty.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ntty.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/ntty

NTTY = ntty.cf/Driver.o
LFILE = $(LINTDIR)/ntty.ln

FILES = \
	ntty.o

CFILES = \
	ntty.c

SRCFILES = $(CFILES)

LFILES = \
	ntty.ln

all: $(NTTY)

install: all
	(cd ntty.cf; $(IDINSTALL) -R$(CONF) -M ntty)

$(NTTY): $(FILES)
	$(LD) -r -o $(NTTY) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(NTTY)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ntty

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
