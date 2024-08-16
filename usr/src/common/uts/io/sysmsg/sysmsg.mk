#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/sysmsg/sysmsg.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sysmsg.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/sysmsg

SYSMSG = sysmsg.cf/Driver.o
LFILE = $(LINTDIR)/sysmsg.ln

FILES = \
	sysmsg.o

CFILES = \
	sysmsg.c

LFILES = \
	sysmsg.ln

all: $(SYSMSG)

install: all
	(cd sysmsg.cf; $(IDINSTALL) -R$(CONF) -M sysmsg)

$(SYSMSG): $(FILES)
	$(LD) -r -o $(SYSMSG) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SYSMSG)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sysmsg 

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
