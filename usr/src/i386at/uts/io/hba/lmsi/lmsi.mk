#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/lmsi/lmsi.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	lmsi.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/lmsi

LMSI = lmsi.cf/Driver.o
CM205 = cm205.cf/Driver.o
LFILE = $(LINTDIR)/lmsi.ln

FILES = hba.o lmsi.o lmsiscsi.o scsifake.o

CFILES = hba.c lmsi.c lmsiscsi.c scsifake.c
LFILES = hba.ln lmsi.ln lmsiscsi.ln scsifake.ln

SRCFILES = $(CFILES)

all:	$(LMSI) $(CM205)

install:	all
		(cd lmsi.cf ; $(IDINSTALL) -R$(CONF) -M lmsi; \
		rm -f $(CONF)/pack.d/lmsi/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/lmsi	) && \
		(cd cm205.cf ; $(IDINSTALL) -R$(CONF) -M cm205)

$(LMSI):	$(FILES)
		$(LD) -r -o $(LMSI) $(FILES)

$(CM205):	cm205.o
		$(LD) -r -o $(CM205) cm205.o

clean:
	-rm -f *.o $(LFILES) *.L $(LMSI)
	-rm -f *.o $(LFILES) *.L $(CM205)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e lmsi
	$(IDINSTALL) -R$(CONF) -d -e cm205

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
	lmsi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
