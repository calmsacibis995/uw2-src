#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/hba.mk	1.27"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	hba.mk
KBASE = ../..

SUBDIRS = athd dcd adsc ictha dpt wd7000  mcis mcesdi mcst mitsumi adsa adse adss ida cpqsc sony lmsi dak blc iiop efp2 fdeb fdsb

all:	FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	 done

install: FRC
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

clobber:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	 done


lintit:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	 done

fnames:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
		fi; \
	done

FRC:

headinstall: FRC
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) hba.h
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
		fi; \
	 done

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done

include $(MAKEFILE).dep
