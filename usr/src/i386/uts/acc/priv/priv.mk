#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:acc/priv/priv.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	priv.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/priv

SUBDIRS = sum lpm

all:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk all"; \
		( cd $$i; $(MAKE) -f $$i.mk all $(MAKEARGS)	); \
	done

install:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk install"; \
		( cd $$i; $(MAKE) -f $$i.mk install $(MAKEARGS) ); \
	done

clean:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk clean"; \
		( cd $$i; $(MAKE) -f $$i.mk clean $(MAKEARGS) ); \
	done

clobber: clean FRC
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk clobber"; \
		( cd $$i; $(MAKE) -f $$i.mk clobber $(MAKEARGS) ); \
	done
	
lintit:
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk lintit"; \
		( cd $$i; $(MAKE) -f $$i.mk lintit $(MAKEARGS) ); \
	done

fnames:
	@for i in $(SUBDIRS);\
	do\
		( \
		cd $$i;\
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;" ; \
		) \
	done

headinstall: localhead FRC
	@for i in $(SUBDIRS); \
	do \
		echo "====== $(MAKE) -f $$i.mk headinstall"; \
		( cd $$i; $(MAKE) -f $$i.mk headinstall $(MAKEARGS) ); \
	done

sysHeaders = \
	privilege.h

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
