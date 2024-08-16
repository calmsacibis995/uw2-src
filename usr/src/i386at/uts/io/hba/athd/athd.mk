#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/athd/athd.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	athd.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/athd

ATHD = athd.cf/Driver.o
ATHDSTUB = athd.cf/Modstub.o
LFILE = $(LINTDIR)/athd.ln

ASFLAGS = -m

FILES = athd.o
CFILES = athd.c
LFILES = athd.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(ATHD) $(ATHDSTUB)

install:	all
		( \
		cd athd.cf ; $(IDINSTALL) -R$(CONF) -M athd; \
		rm -f $(CONF)/pack.d/athd/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/athd/  \
		)

$(ATHD):	$(FILES)
		$(LD) -r -o $(ATHD) $(FILES)

$(ATHDSTUB):	athd_stub.o
		$(LD) -r -o $(ATHDSTUB) athd_stub.o

clean:
	-rm -f *.o $(LFILES) *.L $(ATHD) $(ATHDSTUB)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e athd

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
	athd.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
