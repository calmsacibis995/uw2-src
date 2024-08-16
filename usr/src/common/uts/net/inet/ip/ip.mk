#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/ip/ip.mk	1.5"
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

MAKEFILE=	ip.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/ip

IP = ip.cf/Driver.o

MODULES = $(IP)

FILES = ip_input.o ip_f.o ip_output.o ip_main.o ip_vers.o

LFILE = $(LINTDIR)/ip.ln
LFILES = ip_input.ln ip_f.ln ip_output.ln ip_main.ln ip_vers.ln

CFILES = ip_input.c ip_f.c ip_output.c ip_main.c ip_vers.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd ip.cf; $(IDINSTALL) -R$(CONF) -M ip)

$(IP):	$(FILES)
	$(LD) -r -o $(IP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(IP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ip

$(LINTDIR):
	-mkdir -p $@

ip.klint: $(SRCFILES)

klintit: ip.klint
	klint $(SRCFILES) >ip.klint 2>&1 || true

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
	ip.h \
	ip_f.h \
	ip_str.h \
	ip_var.h \
	ip_var_f.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
