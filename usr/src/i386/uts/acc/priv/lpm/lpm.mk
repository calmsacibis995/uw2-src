#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:acc/priv/lpm/lpm.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	lpm.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/priv/lpm

LPM = lpm.cf/Driver.o
LFILE = $(LINTDIR)/lpm.ln

FILES = \
	lpm.o

CFILES = \
	lpm.c

SRCFILES = $(CFILES)

LFILES = \
	lpm.ln

all:	$(LPM)

install: all
	(cd lpm.cf; $(IDINSTALL) -R$(CONF) -M lpm)

$(LPM): $(FILES)
	$(LD) -r -o $(LPM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(LPM)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e lpm
	
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

headinstall: \
	$(KBASE)/acc/priv/lpm/lpm.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/priv/lpm/lpm.h

include $(UTSDEPEND)

include $(MAKEFILE).dep
