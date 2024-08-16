#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:io/ssm/ssm.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ssm.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/ssm

SSM = ssm.cf/Driver.o
LFILE = $(LINTDIR)/ssm.ln

FILES = \
	ssm_cmblck.o \
	ssm.o \
	ssm_misc.o \
	ssm_scsi.o \
	ssm_vme.o

CFILES = \
	ssm_cmblck.c \
	ssm.c \
	ssm_misc.c \
	ssm_scsi.c \
	ssm_vme.c

SRCFILES = $(CFILES)

LFILES = \
	ssm_cmblck.ln \
	ssm.ln \
	ssm_misc.ln \
	ssm_scsi.ln \
	ssm_vme.ln


all: $(SSM)

install: all
	(cd ssm.cf; $(IDINSTALL) -R$(CONF) -M ssm)

$(SSM): $(FILES)
	$(LD) -r -o $(SSM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ssm

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
	ssm.h \
	ssm_cb.h \
	ssm_cmblck.h \
	ssm_misc.h \
	ssm_scsi.h \
	ssm_vme.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
