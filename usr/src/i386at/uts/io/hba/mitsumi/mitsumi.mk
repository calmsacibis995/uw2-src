#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/mitsumi/mitsumi.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mitsumi.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/mitsumi

MITSUMI = mitsumi.cf/Driver.o
LFILE = $(LINTDIR)/mitsumi.ln

FILES = hba.o mitsumi.o mitsumiscsi.o scsifake.o

CFILES = hba.c mitsumi.c mitsumiscsi.c scsifake.c
LFILES = hba.ln mitsumi.ln mitsumiscsi.ln scsifake.ln

SRCFILES = $(CFILES)

all:	$(MITSUMI)

install:	all
		(cd mitsumi.cf ; $(IDINSTALL) -R$(CONF) -M mitsumi; \
		rm -f $(CONF)/pack.d/mitsumi/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/mitsumi	)

$(MITSUMI):	$(FILES)
		$(LD) -r -o $(MITSUMI) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(MITSUMI)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mitsumi

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
	mitsumi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
