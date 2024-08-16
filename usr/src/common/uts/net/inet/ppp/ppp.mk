#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/ppp/ppp.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ppp.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/ppp

LOCALDEF=-DTCPCOMPRESSION

PPP = ppp.cf/Driver.o

MODULES = $(PPP)

FILES = ppp_pap.o ppp_config.o ppp_ctrl.o ppp_state.o ppp_main.o ppp_log.o

LFILE = $(LINTDIR)/ppp.ln

LFILES = ppp_pap.ln ppp_config.ln ppp_ctrl.ln ppp_state.ln ppp_main.ln ppp_log.ln

CFILES = ppp_pap.c ppp_config.c ppp_ctrl.c ppp_state.c ppp_main.c ppp_log.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd ppp.cf; $(IDINSTALL) -R$(CONF) -M ppp)

$(PPP):	$(FILES)
	$(LD) -r -o $(PPP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(PPP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ppp

$(LINTDIR):
	-mkdir -p $@

ppp.klint: $(SRCFILES)

klintit: ppp.klint
	klint $(SRCFILES) >ppp.klint 2>&1 || true

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do \
		echo $$i; \
	done

netinetHeaders = \
	ppp.h \
	pppcnf.h

headinstall: $(netinetHeaders)
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
