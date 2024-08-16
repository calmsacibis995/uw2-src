#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/mem/mem.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mem.mk
KBASE = ../..
DIR = io/mem
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/mm.ln
MEM = mm.cf/Driver.o

FILES = mem.o
CFILES = mem.c
LFILES = mem.ln

SRCFILES = $(CFILES)


all:	$(MEM)
install: all
	cd mm.cf; $(IDINSTALL) -R$(CONF) -M mm

$(MEM):	$(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f $(FILES) $(MEM)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d mm
	-rm -f $(LFILE) $(LFILES) *.L

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
