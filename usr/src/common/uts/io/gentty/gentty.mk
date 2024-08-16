#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/gentty/gentty.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	gentty.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/gentty

GENTTY = gentty.cf/Driver.o
LFILE = $(LINTDIR)/gentty.ln

FILES = \
	gentty.o

CFILES = \
	gentty.c

LFILES = \
	gentty.ln

all: $(GENTTY)

install: all
	(cd gentty.cf; $(IDINSTALL) -R$(CONF) -M gentty)

$(GENTTY): $(FILES)
	$(LD) -r -o $(GENTTY) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(GENTTY)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e gentty

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
