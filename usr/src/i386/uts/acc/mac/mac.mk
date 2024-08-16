#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:acc/mac/mac.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mac.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/mac

MAC = mac.cf/Driver.o
LFILE = $(LINTDIR)/mac.ln

FILES = \
	covert.o \
	ipcmac.o \
	genmac.o \
	procmac.o \
	vnmac.o 

CFILES = \
	covert.c \
	ipcmac.c \
	genmac.c \
	procmac.c \
	vnmac.c 

SRCFILES = $(CFILES)

LFILES = \
	covert.ln \
	ipcmac.ln \
	genmac.ln \
	procmac.ln \
	vnmac.ln 

all:	$(MAC)

install: all
	(cd mac.cf; $(IDINSTALL) -R$(CONF) -M mac)

$(MAC): $(FILES)
	$(LD) -r -o $(MAC) $(FILES)
clean:
	-rm -f *.o $(LFILES) *.L $(MAC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mac

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
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	covert.h \
        mac_hier.h \
	mac.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
