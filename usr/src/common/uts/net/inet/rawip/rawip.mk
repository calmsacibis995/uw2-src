#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/rawip/rawip.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	rawip.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/rawip

RAWIP = rawip.cf/Driver.o

MODULES = $(RAWIP)

FILES = rawip_main.o rawip.o rawip_cb.o

LFILES = rawip_main.ln rawip.ln rawip_cb.ln

LFILE = $(LINTDIR)/rawip.ln

CFILES = rawip_main.c rawip.c rawip_cb.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd rawip.cf; $(IDINSTALL) -R$(CONF) -M rawip)

$(RAWIP):	$(FILES)
	$(LD) -r -o $(RAWIP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(RAWIP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e rawip

$(LINTDIR):
	-mkdir -p $@

rawip.klint: $(SRCFILES)

klintit:	rawip.klint
	klint $(SRCFILES) >rawip.klint 2>&1 || true

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
