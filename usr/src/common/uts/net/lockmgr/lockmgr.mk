#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/lockmgr/lockmgr.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	lockmgr.mk
KBASE =	../..
LINTDIR=$(KBASE)/lintdir
DIR = net/lockmgr

KLM=klm.cf/Driver.o
LFILE=$(LINTDIR)/klm.ln

MODULES = \
	$(KLM)

FILES = klm_kprot.o \
	klm_lkmgr.o

LFILES = klm_kprot.ln \
	klm_lkmgr.ln

CFILES = klm_kprot.c \
	klm_lkmgr.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd klm.cf; $(IDINSTALL) -R$(CONF) -M klm)

$(KLM): $(FILES)
	$(LD) -r -o $(KLM) $(FILES)

clean:
	rm -f *.o $(LFILES) *.L $(KLM)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e klm

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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

klmHeaders = \
	klm_prot.h \
	lockmgr.h

headinstall: $(klmHeaders)
	@-[ -d $(INC)/klm ] || mkdir -p $(INC)/klm
	@for f in $(klmHeaders); \
	 do \
		$(INS) -f $(INC)/klm -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
