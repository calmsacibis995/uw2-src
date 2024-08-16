#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:util/kdb/kdb/kdb.mk	1.16"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	kdb.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = util/kdb/kdb

KDB = kdb.cf/Driver.o
LFILE = $(LINTDIR)/kdb.ln

FILES = \
	db.o \
	dbag.o \
	dbcon.o \
	dbintrp.o \
	dblex.o \
	dbpsinfo.o \
	dbmisc.o

CFILES = \
	db.c \
	dbag.c \
	dbcon.c \
	dbintrp.c \
	dblex.c \
	dbpsinfo.c \
	dbmisc.c

SRCFILES = $(CFILES)

LFILES = \
	db.ln \
	dbag.ln \
	dbcon.ln \
	dbintrp.ln \
	dblex.ln \
	dbpsinfo.ln \
	dbmisc.ln


all:	$(KDB)

install: all
	cd kdb.cf; $(IDINSTALL) -M -R$(CONF) kdb

$(KDB):  $(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(KDB)

clobber:	clean
	$(IDINSTALL) -R$(CONF) -d -e kdb

lintit: $(LFILE)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

$(LFILE): $(LINTDIR) $(LFILES)

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
