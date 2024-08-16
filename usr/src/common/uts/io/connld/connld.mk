#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/connld/connld.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	connld.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/connld

CONNLD = connld.cf/Driver.o
LFILE = $(LINTDIR)/connld.ln

FILES = \
	connld.o

CFILES = \
	connld.c

SRCFILES = $(CFILES)

LFILES = \
	connld.ln


all: $(CONNLD)

install: all
	(cd connld.cf; $(IDINSTALL) -R$(CONF) -M connld)

$(CONNLD): $(FILES)
	$(LD) -r -o $(CONNLD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(CONNLD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e connld

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
