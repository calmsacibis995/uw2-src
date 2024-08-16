#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/specfs/specfs.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	specfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/specfs

SPECFS = specfs.cf/Driver.o
LFILE = $(LINTDIR)/specfs.ln

MODULES = \
	$(SPECFS)


FILES = specdata.o \
	specsec.o \
	specsubr.o \
	specvfsops.o \
	specvnops.o

LFILES = specdata.ln \
	specsec.ln \
	specsubr.ln \
	specvfsops.ln \
	specvnops.ln

CFILES = specdata.c \
	specsec.c \
	specsubr.c \
	specvfsops.c \
	specvnops.c

SRCFILES = $(CFILES)


all:	$(MODULES)

install: all
	cd specfs.cf; $(IDINSTALL) -R$(CONF) -M specfs

$(SPECFS): $(FILES)
	$(LD) -r -o $(SPECFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SPECFS)

clobber: clean 
	-$(IDINSTALL) -R$(CONF) -d -e specfs

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
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

sysfsHeaders = \
	snode.h \
	devmac.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
