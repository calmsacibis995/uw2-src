#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/kbd/kbd.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	kbd.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/kbd

KBD = kbd.cf/Driver.o
LFILE = $(LINTDIR)/kbd.ln

FILES = \
	kbd.o

CFILES = \
	kbd.c

SRCFILES = $(CFILES)

LFILES = \
	kbd.ln

all: $(KBD)

install: all
	(cd kbd.cf; $(IDINSTALL) -R$(CONF) -M kbd)

$(KBD): $(FILES)
	$(LD) -r -o $(KBD) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(KBD)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e kbd

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
	kbd.h \
	kbduser.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
