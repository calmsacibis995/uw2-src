#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/dcd/dcd.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	dcd.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/dcd

DCD = dcd.cf/Driver.o
LFILE = $(LINTDIR)/dcd.ln

FILES = dcd.o dcdhlpr.o gendev.o
CFILES = dcd.c dcdhlpr.c gendev.c
LFILES = dcd.ln dcdhlpr.ln gendev.ln

SRCFILES = $(CFILES)

all:	$(DCD)

install:	all
		( \
		cd dcd.cf ; $(IDINSTALL) -R$(CONF) -M dcd; \
		rm -f  $(CONF)/pack.d/dcd/space.gen;  \
		cp space.gen $(CONF)/pack.d/dcd/ \
		)


$(DCD):		$(FILES)
		$(LD) -r -o $(DCD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(DCD)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e dcd

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


sysHeaders = \
	dcd.h  \
	gendev.h \
	gendisk.h \
	gentape.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
