#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/icmp/icmp.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	icmp.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/icmp

ICMP = icmp.cf/Driver.o

MODULES = $(ICMP)

FILES = icmp_main.o

LFILE = $(LINTDIR)/icmp.ln

LFILES = icmp_main.ln

CFILES = icmp_main.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd icmp.cf; $(IDINSTALL) -R$(CONF) -M icmp)

$(ICMP):	$(FILES)
	$(LD) -r -o $(ICMP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(ICMP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e icmp

$(LINTDIR):
	-mkdir -p $@

icmp.klint: $(SRCFILES)

klintit: icmp.klint
	klint $(SRCFILES) >icmp.klint 2>&1 || true

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

netinetHeaders = \
	icmp_var.h \
	ip_icmp.h \
	ip_icmp_f.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
