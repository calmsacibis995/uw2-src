#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/inet/app/app.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	app.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/inet/app

APP = app.cf/Driver.o
LFILE = $(LINTDIR)/app.ln

MODULES = $(APP)

FILES = app.o

LFILES = app.ln

CFILES = app.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd app.cf; $(IDINSTALL) -R$(CONF) -M app)

$(APP):	$(FILES)
	$(LD) -r -o $(APP) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L *.klint $(APP)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e app

$(LINTDIR):
	-mkdir -p $@

app.klint: $(SRCFILES)

klintit: app.klint
	klint $(SRCFILES) >app.klint 2>&1 || true

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

headinstall:

include $(UTSDEPEND)

include $(MAKEFILE).dep
