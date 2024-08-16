#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-nuc:io/NWam/NWam.mk	1.5"

#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/io/NWam/NWam.mk,v 1.5 1994/09/20 23:56:16 ram Exp $"



include $(UTSRULES)

MAKEFILE=	NWam.mk

KBASE    = ../..

LINTDIR = $(KBASE)/lintdir
DIR = io/NWam

NWAM = NWam.cf/Driver.o
LFILE = $(LINTDIR)/NWam.ln

FILES = NWam.o

CFILES = NWam.c

SRCFILES = $(CFILES)

LFILES = NWam.ln

all:	$(NWAM)

install: all
	(cd NWam.cf; $(IDINSTALL) -R$(CONF) -M NWam)

$(NWAM): $(FILES)
	$(LD) -r -o $(NWAM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(NWAM)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e NWam

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
	nwam.h 

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

# include $(MAKEFILE).dep
