#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/dpt/dpt.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	dpt.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/dpt

DPT = dpt.cf/Driver.o
LFILE = $(LINTDIR)/dpt.ln

ASFLAGS = -m

FILES = dpt.o
CFILES = dpt.c
LFILES = dpt.ln

SRCFILES = $(CFILES)

all:	$(DPT)

install:	all
		( \
		cd dpt.cf ; $(IDINSTALL) -R$(CONF) -M dpt; \
		rm -f  $(CONF)/pack.d/dpt/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/dpt/ \
		)


$(DPT):	$(FILES)
		$(LD) -r -o $(DPT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(DPT)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e dpt

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
	dpt.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
