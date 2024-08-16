#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/dak/dak.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	dak.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/dak

DAK = dak.cf/Driver.o
LFILE = $(LINTDIR)/dak.ln

ASFLAGS = -m

FILES = dak.o
CFILES = dak.c
LFILES = dak.ln

SRCFILES = $(CFILES)

all:	$(DAK)

install:	all
		( \
		cd dak.cf ; $(IDINSTALL) -R$(CONF) -M dak; \
		rm -f  $(CONF)/pack.d/dak/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/dak/ \
		)


$(DAK):	$(FILES)
		$(LD) -r -o $(DAK) $(FILES)

dak.o: dak.c dak.h

FRC:

clean:
	-rm -f *.o $(LFILES) *.L $(DAK)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e dak

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
	dak.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
