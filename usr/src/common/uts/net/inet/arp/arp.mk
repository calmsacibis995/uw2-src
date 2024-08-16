#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/arp/arp.mk	1.4"
#ident	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

include $(UTSRULES)

MAKEFILE=	arp.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/arp

ARP = arp.cf/Driver.o
LFILE = $(LINTDIR)/arp.ln

MODULES = $(ARP)

FILES = arp.o

LFILES = arp.ln

CFILES = arp.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd arp.cf; $(IDINSTALL) -R$(CONF) -M arp)

$(ARP):	$(FILES)
	$(LD) -r -o $(ARP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(ARP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e arp

$(LINTDIR):
	-mkdir -p $@

arp.klint: $(SRCFILES)

klintit:	arp.klint
	klint $(SRCFILES) >arp.klint 2>&1 || true

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

netHeaders = \
	arp.h

headinstall: $(netHeaders)
	@-[ -d $(INC)/net ] || mkdir -p $(INC)/net
	@for f in $(netHeaders); \
	 do \
	    $(INS) -f $(INC)/net -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
