#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/autoconf/autoconf.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = autoconf.mk
KBASE = ../..
DIR = io/autoconf

SUBDIRS = resmgr confmgr ca

all:	FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	done

install: FRC
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

clobber: localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	done

localclobber: localclean

lintit: $(LFILE)
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	done

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

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend"; \
		 touch $$d.mk.dep; \
		 $(MAKE) -f $$d.mk depend $(MAKEARGS)); \
	done

FRC:
