#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/nw.mk	1.3"
#ident	"$Id: nw.mk,v 1.3 1994/02/11 20:48:22 vtag Exp $"

include $(UTSRULES)

include local.defs

MAKEFILE=	nw.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw

SUBDIRS = ipx ripx spx 

all:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

install:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

clean:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

clobber:	clean FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

lintit:	$(LFILE)

fnames:
	@for d in $(SUBDIRS); do \
		(cd $$d; \
			$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
			$(SED) -e "s;^;$$d/;"); \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	diag_app.h \
	ipx_app.h \
	nwtdr.h \
	nwportable.h \
	spx_app.h \
	ipxspx_diag.h \
	lipmx_app.h \
	ripx_app.h \
	sap.h \
	sap_dos.h \
	sap_app.h

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
