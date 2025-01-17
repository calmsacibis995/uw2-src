#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/tirdwr/tirdwr.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	tirdwr.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/tirdwr

TIRDWR = tirdwr.cf/Driver.o
LFILE = $(LINTDIR)/tirdwr.ln

FILES = \
	tirdwr.o

CFILES = \
	tirdwr.c

SRCFILES = $(CFILES)

LFILES = \
	tirdwr.ln

all: $(TIRDWR)

install: all
	(cd tirdwr.cf; $(IDINSTALL) -R$(CONF) -M tirdwr)

$(TIRDWR): $(FILES)
	$(LD) -r -o $(TIRDWR) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(TIRDWR)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e tirdwr

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
