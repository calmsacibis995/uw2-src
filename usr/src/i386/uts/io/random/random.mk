#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)kern-i386:io/random/random.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	random.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/random

LOCALDEF = -DDDI_OFF

RANDOM = random.cf/Driver.o
LFILE = $(LINTDIR)/random.ln

FILES = \
	random.o 

CFILES = \
	random.c 

LFILES = \
	random.ln

all: $(RANDOM)

install: all
	(cd random.cf; $(IDINSTALL) -R$(CONF) -M rand)

$(RANDOM): $(FILES)
	$(LD) -r -o $(RANDOM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(RANDOM)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e io

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
