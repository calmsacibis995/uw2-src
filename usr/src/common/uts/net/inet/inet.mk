#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/inet.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	inet.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet

INET = inet.cf/Driver.o
LFILE = $(LINTDIR)/inet.ln

FILES = in.o in_pcb.o netlib.o in_cksum.o netlib_f.o in_transp.o
CFILES = in.c in_pcb.c netlib.c netlib_f.c in_transp.c
SFILES = in_cksum.s
SRCFILES = $(CFILES) $(SFILES)
LFILES = in.ln in_pcb.ln netlib.ln netlib_f.ln in_transp.ln

SUBDIRS = app arp asyh llcloop ip icmp ppp rawip route slip tcp udp

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		$(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(INET)

$(INET): $(FILES)
	$(LD) -r -o $(INET) $(FILES)

clean: localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		$(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L *.klint $(INET)

clobber:	localclobber clean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		$(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e inet

install: local FRC
	cd inet.cf; $(IDINSTALL) -R$(CONF) -M inet
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		$(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	byteorder.h \
	byteorder_f.h \
	protosw.h

netHeaders = \
	af.h \
	if.h \
	if_arp.h \
	if_arp_f.h \
	strioc.h

netinetHeaders = \
	if_ether.h \
	if_ether_f.h \
	in.h \
	in_f.h \
	in_pcb.h \
	in_comp.h \
	in_systm.h \
	in_systm_f.h \
	in_var.h \
	insrem.h \
	insrem_f.h \
	nihdr.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/net ] || mkdir -p $(INC)/net
	@for f in $(netHeaders); \
	 do \
	    $(INS) -f $(INC)/net -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/netinet ] || mkdir -p $(INC)/netinet
	@for f in $(netinetHeaders); \
	 do \
	    $(INS) -f $(INC)/netinet -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

klintit:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk klintit"; \
		 $(MAKE) -f $$d.mk klintit $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for d in $(SUBDIRS); do \
		(cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
