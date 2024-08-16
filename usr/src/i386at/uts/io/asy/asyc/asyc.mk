#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/asy/asyc/asyc.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	asyc.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/asy/asyc

ASYC = asyc.cf/Driver.o
LFILE = $(LINTDIR)/asyc.ln

FILES = \
	asyc.o

CFILES = \
	asyc.c


LFILES = \
	asyc.ln


all: $(ASYC)

install: all
	(cd asyc.cf; $(IDINSTALL) -R$(CONF) -M asyc)

$(ASYC): $(FILES)
	$(LD) -r -o $(ASYC) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ASYC)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e asyc 

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	asyc.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
