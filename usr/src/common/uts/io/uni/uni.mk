#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/uni/uni.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	uni.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/uni

UNI = uni.cf/Driver.o
LFILE = $(LINTDIR)/uni.ln

FILES = \
	uni.o

CFILES = \
	uni.c

SRCFILES = $(CFILES)

LFILES = \
	uni.ln


all: $(UNI)

install: all
	(cd uni.cf; $(IDINSTALL) -R$(CONF) -M uni)

$(UNI): $(FILES)
	$(LD) -r -o $(UNI) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(UNI)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e uni

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

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
