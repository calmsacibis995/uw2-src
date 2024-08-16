#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:io/intmap/intmap.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	intmap.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/intmap

INTMAP = intmap.cf/Driver.o
LFILE = $(LINTDIR)/intmap.ln

FILES = \
	nmap.o \
	emap.o \
	xmap.o \
	chanmap.o

CFILES = \
	nmap.c \
	emap.c \
	xmap.c \
	chanmap.c

SRCFILES = $(CFILES)

LFILES = \
	nmap.ln \
	emap.ln \
	xmap.ln \
	chanmap.ln

all: $(INTMAP)

install: all
	(cd intmap.cf; $(IDINSTALL) -R$(CONF) -M intmap)

$(INTMAP): $(FILES)
	$(LD) -r -o $(INTMAP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(INTMAP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e intmap

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
	emap.h \
	nmap.h \
	xmap.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
