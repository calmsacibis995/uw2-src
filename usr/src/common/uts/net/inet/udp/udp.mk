#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/udp/udp.mk	1.4"
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

MAKEFILE=	udp.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/udp

UDP = udp.cf/Driver.o
LFILE = $(LINTDIR)/udp.ln

MODULES = $(UDP)

FILES = udp_io.o udp_main.o udp_state.o
LFILES = udp_io.ln udp_main.ln udp_state.ln

CFILES = udp_io.c udp_main.c udp_state.c
HFILES = udp.h

SRCFILES = $(CFILES)


all:	$(MODULES)

install: all
	(cd udp.cf; $(IDINSTALL) -R$(CONF) -M udp)

$(UDP):	$(FILES)
	$(LD) -r -o $(UDP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(UDP)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e udp

$(LINTDIR):
	-mkdir -p $@

udp.klint: $(SRCFILES)

klintit: udp.klint
	klint $(SRCFILES) >udp.klint 2>&1 || true

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
	udp.h \
	udp_f.h \
	udp_var.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
