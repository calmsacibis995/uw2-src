#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/xpoll/xpoll.mk	1.1"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = xpoll.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/xpoll

XPOLL = xpoll.cf/Driver.o
LFILE = $(LINTDIR)/xpoll.ln

FILES = \
	xpoll.o

CFILES = \
	xpoll.c

SRCFILES = $(CFILES)

LFILES = \
	xpoll.ln


all: $(XPOLL)

install: all
	(cd xpoll.cf; $(IDINSTALL) -R$(CONF) -M xpoll)

$(XPOLL): $(FILES)
	$(LD) -r -o $(XPOLL) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(XPOLL)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e xpoll

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
