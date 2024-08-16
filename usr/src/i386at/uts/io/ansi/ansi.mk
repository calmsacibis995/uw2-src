#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/ansi/ansi.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ansi.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/ansi

ANSI = ansi.cf/Driver.o
LFILE = $(LINTDIR)/ansi.ln

FILES = \
	ansi.o

CFILES = \
	ansi.c

LFILES = \
	ansi.ln

all:	$(ANSI)

install: all
	(cd ansi.cf; $(IDINSTALL) -R$(CONF) -M ansi)

$(ANSI): $(FILES)
	$(LD) -r -o $(ANSI) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ANSI)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ansi

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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


#
# Header Install Section
#
sysHeaders = \
	ansi.h \
	at_ansi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
