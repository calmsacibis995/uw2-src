#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:io/wd/wd.mk	1.10"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	wd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/wd

WD = wd.cf/Driver.o
LFILE = $(LINTDIR)/wd.ln

FILES = \
	wd.o \
	dsort.o

CFILES = \
	wd.c \
	dsort.c

SRCFILES = $(CFILES)

LFILES = \
	wd.ln \
	dsort.ln


all: $(WD)

install: all
	(cd wd.cf; $(IDINSTALL) -R$(CONF) -M wd)

$(WD): $(FILES)
	$(LD) -r -o $(WD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e wd

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
	wd.h \
	dsort.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep