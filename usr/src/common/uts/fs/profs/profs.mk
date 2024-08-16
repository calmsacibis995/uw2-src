#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/profs/profs.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	profs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/profs

PROCESSORFS = profs.cf/Driver.o
LFILE = $(LINTDIR)/profs.ln

MODULES = \
	$(PROCESSORFS)

FILES = profs_vfsops.o \
	profs_vnops.o \
	profs_mdep.o

LFILES = profs_vfsops.ln \
	profs_vnops.ln \
	profs_mdep.ln

CFILES = profs_vfsops.c \
	profs_vnops.c \
	profs_mdep.c

all:	$(MODULES)

install: all
	cd profs.cf; $(IDINSTALL) -R$(CONF) -M processorfs

$(PROCESSORFS): $(FILES)
	$(LD) -r -o $(PROCESSORFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PROCESSORFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e processorfs

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
	@for i in $(CFILES);	\
	do \
		echo $$i; \
	done

sysHeaders = \
	prosrfs.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
