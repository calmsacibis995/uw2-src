#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/adse/adse.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	adse.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/adse
#LOCALDEF = -DAHA_DEBUG3

ADSE = adse.cf/Driver.o
LFILE = $(LINTDIR)/adse.ln

FILES = adse.o

CFILES = adse.c

SFILES =

LFILES = adse.ln

SRCFILES = $(CFILES) $(SFILES)

all:	$(ADSE)

install:	all
		(cd adse.cf ; $(IDINSTALL) -R$(CONF) -M adse; \
		rm -f $(CONF)/pack.d/adse/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/adse	)

$(ADSE):	$(FILES)
		$(LD) -r -o $(ADSE) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ADSE)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e adse

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
	adse.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
