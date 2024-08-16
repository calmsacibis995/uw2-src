#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/mse/bmse/bmse.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	bmse.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mse/bmse

LOCALDEF = -DESMP

BMSE = bmse.cf/Driver.o
LFILE = $(LINTDIR)/bmse.ln

FILES = \
	bmse.o

CFILES = \
	bmse.c

LFILES = \
	bmse.ln


all: $(BMSE)

install: all
	(cd bmse.cf; $(IDINSTALL) -R$(CONF) -M bmse)

$(BMSE): $(FILES)
	$(LD) -r -o $(BMSE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(BMSE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e bmse

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