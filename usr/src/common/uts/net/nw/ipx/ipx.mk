#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/ipx/ipx.mk	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nw/ipx/ipx.mk,v 1.1 1994/01/28 17:48:10 vtag Exp $"

include $(UTSRULES)

include ../local.defs

MAKEFILE=	ipx.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ipx

#SUBDIRS = drvload lipmx ipxr 

SUBDIRS = drvload lipmx ipxs 

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
	(cd ipx.cf; $(IDINSTALL) -R$(CONF) -M ipx)

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
	-$(IDINSTALL) -R$(CONF) -d -e ipx

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

headinstall: FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
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
