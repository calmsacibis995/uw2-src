#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/asyh/asyh.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	asyh.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/asyh

LOCALDEF=-DTCPCOMPRESSION

ASYH = asyh.cf/Driver.o

MODULES = $(ASYH)

FILES = asyh_main.o

LFILE = $(LINTDIR)/asyh_main.ln

LFILES = asyh_main.ln

CFILES = asyh_main.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd asyh.cf; $(IDINSTALL) -R$(CONF) -M asyh)

$(ASYH):	$(FILES)
	$(LD) -r -o $(ASYH) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(ASYH)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e asyh

$(LINTDIR):
	-mkdir -p $@

asyh.klint: $(SRCFILES)

klintit: asyh.klint
	klint $(SRCFILES) >asyh.klint 2>&1 || true

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
	asyhdlc.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
