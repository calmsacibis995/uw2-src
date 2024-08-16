#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:acc/audit/audit.mk	1.10"
#ident  "$Header: $"

include $(UTSRULES)

MAKEFILE=	audit.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/audit

AUDIT = audit.cf/Driver.o
LFILE = $(LINTDIR)/audit.ln

FILES = \
	auditbuf.o \
	auditctl.o \
	auditdmp.o \
	auditent.o \
	auditevt.o \
	auditflush.o \
	auditlog.o \
	auditrec.o \
	auditsubr.o 

CFILES = \
	auditbuf.c \
	auditctl.c \
	auditdmp.c \
	auditent.c \
	auditevt.c \
	auditflush.c \
	auditlog.c \
	auditrec.c \
	auditsubr.c 

SRCFILES = $(CFILES)

LFILES = \
	auditbuf.ln \
	auditctl.ln \
	auditdmp.ln \
	auditent.ln \
	auditevt.ln \
	auditlog.ln \
	auditflush.ln \
	auditrec.ln \
	auditsubr.ln 

all:	$(AUDIT)

install: all
	(cd audit.cf; $(IDINSTALL) -R$(CONF) -M audit)

$(AUDIT): $(FILES)
	$(LD) -r -o $(AUDIT) $(FILES)
clean:
	-rm -f *.o $(LFILES) *.L $(AUDIT)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e audit

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

headinstall: \
	$(KBASE)/acc/audit/audit.h \
	$(KBASE)/acc/audit/auditmdep.h \
	$(KBASE)/acc/audit/auditrec.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/audit.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/auditmdep.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/audit/auditrec.h



include $(UTSDEPEND)

include $(MAKEFILE).dep
