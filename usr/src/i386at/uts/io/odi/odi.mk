#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/odi.mk	1.3"

include $(UTSRULES)

MAKEFILE=	odi.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
SUBDIRS = lsl msm tsm odisr drv compat

.SUFFIXES: .ln

.c.ln:

all:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		$(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	done

install:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	done

clean:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		$(MAKE) -f $$d.mk clean $(MAKEARGS)); \
		fi; \
	done

clobber: clean
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		$(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	done

$(LINTDIR):

lintit:
	@for d in $(LINTDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		$(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	done

fnames:

headinstall:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		$(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	done

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		touch $$d.mk.dep;\
		$(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done
