#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/dlpi_ether/dlpi_ether.mk	1.18"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	dlpi_ether.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/dlpi_ether

LFILE = $(LINTDIR)/dlpi_ether.ln

SUBDIRS = ee16 el16 imx586 en596 flash32

SRCFILES = dlpi_ether.c

all:	
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

install: all
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

clean:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done


clobber:	clean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done


lintit:
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

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for d in $(SUBDIRS); do \
		(cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

#
# Header Install Section
#

sysHeaders = \
	dlpi_ether.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done


include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
