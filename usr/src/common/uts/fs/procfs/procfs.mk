#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/procfs/procfs.mk	1.13"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	procfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/procfs

PROCFS = procfs.cf/Driver.o
LFILE = $(LINTDIR)/procfs.ln

MODULES = \
	$(PROCFS)

FILES =		prinactive.o \
		prlock.o \
		prlookup.o \
		prmachdep.o \
		prptrace.o \
		prread.o \
		prreaddir.o \
		prsubr.o \
		prusrio.o \
		prvfsops.o \
		prvnops.o \
		prwrite.o

LFILES =	prinactive.ln \
		prlock.ln \
		prlookup.ln \
		prmachdep.ln \
		prptrace.ln \
		prread.ln \
		prreaddir.ln \
		prsubr.ln \
		prusrio.ln \
		prvfsops.ln \
		prvnops.ln \
		prwrite.ln

CFILES =	prinactive.c \
		prlock.c \
		prlookup.c \
		prmachdep.c \
		prptrace.c \
		prread.c \
		prreaddir.c \
		prsubr.c \
		prusrio.c \
		prvfsops.c \
		prvnops.c \
		prwrite.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd procfs.cf; $(IDINSTALL) -R$(CONF) -M procfs)

$(PROCFS): $(FILES)
	$(LD) -r -o $(PROCFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(PROCFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e procfs

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

sysHeaders = \
	procfs.h \
	procfs_f.h
fsprocfsHeaders = \
	prdata.h

headinstall: $(sysHeaders) $(fsprocfsHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/fs/procfs ] || mkdir -p $(INC)/fs/procfs
	@for f in $(fsprocfsHeaders); \
	 do \
	    $(INS) -f $(INC)/fs/procfs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
