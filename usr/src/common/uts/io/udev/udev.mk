#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/udev/udev.mk	1.1"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = udev.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/udev

UDEV = udev.cf/Driver.o
LFILE = $(LINTDIR)/udev.ln

FILES = \
	udev.o

CFILES = \
	udev.c

LFILES = \
	udev.ln


all: $(UDEV)

install: all
	(cd udev.cf; $(IDINSTALL) -R$(CONF) -M udev)

$(UDEV): $(FILES)
	$(LD) -r -o $(UDEV) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(UDEV)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e udev

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

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
