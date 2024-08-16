#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/pt/pt.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	pt.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/pt

PTM = ptm.cf/Driver.o
PTS = pts.cf/Driver.o
PTEM = ptem.cf/Driver.o
PCKT = pckt.cf/Driver.o
LFILE = $(LINTDIR)/pt.ln

PTMFILES = ptm.o

PTSFILES = pts.o

PTEMFILES = ptem.o

PCKTFILES = pckt.o

FILES = ${PTMFILES} ${PTSFILES} ${PTEMFILES} ${PCKTFILES}

CFILES = \
	ptm.c \
	pts.c \
	ptem.c \
	pckt.c

SRCFILES = $(CFILES)

LFILES = \
	ptm.ln \
	pts.ln \
	ptem.ln \
	pckt.ln

all: $(PTM) $(PTS) $(PTEM) $(PCKT) FRC

install: all FRC
	(cd ptm.cf; $(IDINSTALL) -R$(CONF) -M ptm)
	(cd pts.cf; $(IDINSTALL) -R$(CONF) -M pts)
	(cd ptem.cf; $(IDINSTALL) -R$(CONF) -M ptem)
	(cd pckt.cf; $(IDINSTALL) -R$(CONF) -M pckt)

$(PTM): $(PTMFILES)
	$(LD) -r -o $(PTM) $(PTMFILES)

$(PTS): $(PTSFILES)
	$(LD) -r -o $(PTS) $(PTSFILES)

$(PTEM): $(PTEMFILES)
	$(LD) -r -o $(PTEM) $(PTEMFILES)

$(PCKT): $(PCKTFILES)
	$(LD) -r -o $(PCKT) $(PCKTFILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PTM) $(PTS) $(PTEM) $(PCKT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ptm
	-$(IDINSTALL) -R$(CONF) -d -e pts
	-$(IDINSTALL) -R$(CONF) -d -e ptem
	-$(IDINSTALL) -R$(CONF) -d -e pckt

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	ptem.h \
	ptms.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep
