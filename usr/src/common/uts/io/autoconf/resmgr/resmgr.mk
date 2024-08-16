#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/autoconf/resmgr/resmgr.mk	1.1"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = resmgr.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/autoconf/resmgr

RESMGR = resmgr.cf/Driver.o
LFILE = $(LINTDIR)/resmgr.ln

FILES = \
	resmgr.o

CFILES = \
	resmgr.c

LFILES = \
	resmgr.ln


all: $(RESMGR)

install: all
	(cd resmgr.cf; $(IDINSTALL) -R$(CONF) -M resmgr)

$(RESMGR): $(FILES)
	$(LD) -r -o $(RESMGR) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(RESMGR)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e resmgr

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	resmgr.h

headinstall:	$(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
