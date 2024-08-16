#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:psm/psm.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = psm.mk
DIR = psm
KBASE = ..

all:	local FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk all"; \
		 $(MAKE) -f atup.mk all $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
				 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
			fi; \
		done; \
	fi

local:

install: localinstall FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk install"; \
		 $(MAKE) -f atup.mk install $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
				 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
			fi; \
		done; \
	fi

localinstall: local FRC

clean:	localclean FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk clean"; \
		 $(MAKE) -f atup.mk clean $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
				 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
			fi; \
		done; \
	fi

localclean:

localclobber:	localclean

clobber:	localclobber FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk clobber"; \
		 $(MAKE) -f atup.mk clobber $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
				 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
			fi; \
		done; \
	fi

lintit:	FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk lintit"; \
		 $(MAKE) -f atup.mk lintit $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
				 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
			fi; \
		done; \
	fi

fnames:	FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; \
		 $(MAKE) -f atup.mk fnames $(MAKEARGS) | \
		 $(SED) -e "s;^;atup/;"); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; \
				 $(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
				 $(SED) -e "s;^;$$d/;"); \
			fi; \
		done; \
	fi

headinstall: localhead FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk headinstall"; \
		 $(MAKE) -f atup.mk headinstall $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
				 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
			fi; \
		done; \
	fi

localhead:

FRC:

depend::
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		(cd atup; echo "=== $(MAKE) -f atup.mk depend"; \
		 $(MAKE) -f atup.mk depend $(MAKEARGS)); \
	else \
		for d in `/bin/ls`; do \
			if [ -d $$d ]; then \
				(cd $$d; echo "=== $(MAKE) -f $$d.mk depend"; \
				 $(MAKE) -f $$d.mk depend $(MAKEARGS)); \
			fi; \
		done; \
	fi
