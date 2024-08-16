#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/iaf/iaf.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	iaf.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/iaf

IAF = iaf.cf/Driver.o
LFILE = $(LINTDIR)/iaf.ln

FILES = \
	iaf.o

CFILES = \
	iaf.c

SRCFILES = $(CFILES)

LFILES = \
	iaf.ln


all: $(IAF)

install: all
	(cd iaf.cf; $(IDINSTALL) -R$(CONF) -M iaf)

$(IAF): $(FILES)
	$(LD) -r -o $(IAF) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(IAF)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e iaf

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
	iaf.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
