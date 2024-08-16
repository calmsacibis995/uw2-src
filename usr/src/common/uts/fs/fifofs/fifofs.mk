#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/fifofs/fifofs.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	fifofs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/fifofs

FIFOFS = fifofs.cf/Driver.o
LFILE = $(LINTDIR)/fifofs.ln

MODULES = \
	$(FIFOFS)

FILES = fifovnops.o \
	 fifosubr.o

LFILES = fifovnops.ln \
	 fifosubr.ln

CFILES = fifovnops.c \
	 fifosubr.c
SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd fifofs.cf; $(IDINSTALL) -R$(CONF) -M fifofs)

$(FIFOFS):	$(FILES)
	$(LD) -r -o $(FIFOFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(FIFOFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e fifofs

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
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

sysfsHeaders = \
	fifonode.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
