#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/slip/slip.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	slip.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/slip

LOCALDEF=-DTCPCOMPRESSION

SLIP = slip.cf/Driver.o

MODULES = $(SLIP)

FILES = slip.o 

LFILES = slip.ln

LFILE = $(LINTDIR)/slip.ln

CFILES = slip.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd slip.cf; $(IDINSTALL) -R$(CONF) -M slip)

$(SLIP):	$(FILES)
	$(LD) -r -o $(SLIP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(SLIP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e slip

$(LINTDIR):
	-mkdir -p $@

slip.klint: $(SRCFILES)

klintit:	slip.klint
	klint $(SRCFILES) >slip.klint 2>&1 || true

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
	slip.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
