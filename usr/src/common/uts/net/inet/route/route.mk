#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/route/route.mk	1.1"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	route.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/route

ROUTE = route.cf/Driver.o

MODULES = $(ROUTE)

FILES = route.o route_prov.o

LFILE = $(LINTDIR)/route.ln
LFILES = route.ln route_prov.ln

CFILES = route.c route_prov.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd route.cf; $(IDINSTALL) -R$(CONF) -M route)

$(ROUTE):	$(FILES)
	$(LD) -r -o $(ROUTE) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(ROUTE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e route

$(LINTDIR):
	-mkdir -p $@

route.klint: $(SRCFILES)

klintit: route.klint
	klint $(SRCFILES) >route.klint 2>&1 || true

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
	route.h \
	route_kern.h

headinstall: $(netHeaders)
	@-[ -d $(INC)/net ] || mkdir -p $(INC)/net
	@for f in $(netHeaders); \
	 do \
	    $(INS) -f $(INC)/net -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
