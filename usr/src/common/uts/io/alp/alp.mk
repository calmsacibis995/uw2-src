#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/alp/alp.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	alp.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/alp

ALP = alp.cf/Driver.o
LFILE = $(LINTDIR)/alp.ln

FILES = \
	alp.o

CFILES = \
	alp.c

SRCFILES = $(CFILES)

LFILES = \
	alp.ln

all: $(ALP)

install: all
	(cd alp.cf; $(IDINSTALL) -R$(CONF) -M alp)

$(ALP): $(FILES)
	$(LD) -r -o $(ALP) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ALP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e alp

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
	alp.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
