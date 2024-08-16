#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/fdfs/fdfs.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	fdfs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/fdfs

FDFS = fdfs.cf/Driver.o
LFILE = $(LINTDIR)/fdfs.ln

MODULES = \
	$(FDFS)

FILES = fdops.o

LFILES = fdops.ln 

CFILES = fdops.c
SRCFILES = $(CFILES)

all: $(MODULES)

install: all
	(cd fdfs.cf; $(IDINSTALL) -R$(CONF) -M fdfs)

$(FDFS): $(FILES)
	$(LD) -r -o $(FDFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(FDFS)

clobber:        clean
	-$(IDINSTALL) -R$(CONF) -d -e fdfs

$(LINTDIR):
	 -mkdir -p $@


lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	@for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
		do \
			echo $$i; \
		done

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
