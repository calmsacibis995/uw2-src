#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/sad/sad.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sad.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/sad

SAD = sad.cf/Driver.o
LFILE = $(LINTDIR)/sad.ln

FILES = \
	sad.o

CFILES = \
	sad.c

SRCFILES = $(CFILES)

LFILES = \
	sad.ln


all: $(SAD)

install: all
	(cd sad.cf; $(IDINSTALL) -R$(CONF) -M sad)

$(SAD): $(FILES)
	$(LD) -r -o $(SAD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SAD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sad

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
	sad.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
