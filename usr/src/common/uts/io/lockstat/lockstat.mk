#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/lockstat/lockstat.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	lockstat.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/lockstat

LOCKSTAT = lockstat.cf/Driver.o
LFILE = $(LINTDIR)/lockstat.ln

FILES = \
	lockstat.o

CFILES = \
	lockstat.c

SRCFILES = $(CFILES)

LFILES = \
	lockstat.ln

all:	$(LOCKSTAT)

install: all
	(cd lockstat.cf; $(IDINSTALL) -R$(CONF) -M lockstat)
	

$(LOCKSTAT):	$(FILES)
	$(LD) -r -o $(LOCKSTAT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(LOCKSTAT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e lockstat

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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
