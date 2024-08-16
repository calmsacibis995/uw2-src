#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/loopback/ticlts/ticlts.mk	1.6"
#ident 	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ticlts.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/loopback/ticlts
LOCALDEF = -DTICLTS

TICLTS = ticlts.cf/Driver.o
LFILE = $(LINTDIR)/ticlts.ln

CFILES = ticlts.c
FILES = ticlts.o
LFILES = ticlts.ln

SRCFILES = $(CFILES)


all: $(TICLTS)

install: all
	(cd ticlts.cf; $(IDINSTALL) -R$(CONF) -M ticlts)

$(TICLTS): $(FILES)
	$(LD) -r -o $(TICLTS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(TICLTS)

clobber: clean
	$(IDINSTALL) -e -R$(CONF) -d -e ticlts

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


FRC:

headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
