#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/xnamfs/xnamfs.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	xnamfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/xnamfs

XNAMFS = xnamfs.cf/Driver.o
MODSTUB = xnamfs.cf/Modstub.o
LFILE = $(LINTDIR)/xnamfs.ln

MODULES = \
	$(XNAMFS) \
	$(MODSTUB)

FILES = xnamsubr.o \
	xnamvfsops.o \
	xnamvnops.o \
	xsem.o \
	xsd.o

LFILES = xnamsubr.ln \
	xnamvfsops.ln \
	xnamvnops.ln \
	xsem.ln \
	xsd.ln

CFILES = \
	xnamsubr.c \
	xnamvfsops.c \
	xnamvnops.c \
	xsem.c \
	xsd.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd xnamfs.cf; $(IDINSTALL) -R$(CONF) -M xnamfs)

$(XNAMFS): $(FILES)
	$(LD) -r -o $@ $(FILES)

$(MODSTUB): xnamfs_stub.o
	$(LD) -r -o $@ xnamfs_stub.o 

clean:
	-rm -f $(FILES) $(LFILES) *.L xnamfs_stub.o $(XNAMFS) $(MODSTUB)

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d xnamfs

lintit:	$(LFILE)

$(LINTDIR):
	-mkdir -p $@

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do \
		echo $$i; \
	done

sysfsHeaders = \
	xnamnode.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
