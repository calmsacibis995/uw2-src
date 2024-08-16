#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/asy/asy.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	asy.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/asy

IASY = iasy.cf/Driver.o
LFILE = $(LINTDIR)/iasy.ln

FILES = \
	iasy.o

CFILES = \
	iasy.c

SRCFILES = $(CFILES)

LFILES = \
	iasy.ln

SUBDIRS = asyc asyhp

all:	$(IASY) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		$(MAKE) -f $$d.mk all $(MAKEARGS)); \
	done

$(IASY): $(FILES)
	$(LD) -r -o $(IASY) $(FILES)

install: $(IASY) FRC
	cd iasy.cf; $(IDINSTALL) -R$(CONF) -M iasy
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS)); \
	done

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		$(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	done

localclean:
	-rm -f *.o $(LFILES) *.L $(IASY)

clobber: localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		$(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	done

localclobber: localclean
	-$(IDINSTALL) -R$(CONF) -d -e iasy

lintit: $(LFILE)
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk linit"; \
		$(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	done

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
	@for i in $(SUBDIRS); do \
	    (cd $$i; \
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;"); \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		$(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	done

sysHeaders = \
	iasy.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
