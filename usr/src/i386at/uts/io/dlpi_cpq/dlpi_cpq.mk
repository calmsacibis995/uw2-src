#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/dlpi_cpq/dlpi_cpq.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE	= dlpi_cpq.mk
DIRS		= cet pnt

all install:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@ $(MAKEARGS);\
		)\
	done;\
	wait

headinstall clean lintit:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait

clobber:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait

include $(UTSDEPEND)

depend::
	@for d in $(DIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		touch $$d.mk.dep;\
		$(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep

