#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_util.mk	1.16"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	kdb_util.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = util/kdb/kdb_util

KDBUTIL = kdb_util.cf/Driver.o
LFILE = $(LINTDIR)/kdb_util.ln

FILES = \
	bits.o \
	db_as.o \
	extn.o \
	kdb.o \
	kdb_p.o \
	opset.o \
	stacktrace.o \
	tbls.o \
	utls.o

CFILES = \
	bits.c \
	db_as.c \
	extn.c \
	kdb.c \
	kdb_p.c \
	opset.c \
	stacktrace.c \
	tbls.c \
	utls.c

SRCFILES = $(CFILES)

LFILES = \
	bits.ln \
	db_as.ln \
	extn.ln \
	kdb.ln \
	kdb_p.ln \
	opset.ln \
	stacktrace.ln \
	tbls.ln \
	utls.ln


all:	$(KDBUTIL) kdb_util.cf/Modstub.o

install: all
	(cd kdb_util.cf; $(IDINSTALL) -M -R$(CONF) kdb_util)

$(KDBUTIL): $(FILES)
	$(LD) -r -o $(KDBUTIL) $(FILES)

kdb_util.cf/Modstub.o:	kdb_util.cf/Stubs.c kdb_stub.o
	$(CC) -c $(CFLAGS) $(INCLIST) $(DEFLIST) -DMODSTUB kdb_util.cf/Stubs.c
	$(LD) -r -o kdb_util.cf/Modstub.o Stubs.o kdb_stub.o
	-rm -f Stubs.o

clean:
	-rm -f *.o $(LFILES) *.L $(KDBUTIL) kdb_util.cf/Modstub.o

clobber: clean
	$(IDINSTALL) -R$(CONF) -d kdb_util

lintit:	$(LFILE)
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
