#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:common/ktool/ktool.mk	1.9"
#ident	"$Header: $"

#
# makefile for ktool kernel build tools
#

include $(CMDRULES)

all:	idtools.all unixsyms.all locklist.all ddicheck.all

idtools.all:
	@echo $(MAKE) -f idtools.mk all
	@cd idtools; $(MAKE) -f idtools.mk all $(MAKEARGS)
unixsyms.all:
	@echo $(MAKE) -f unixsyms.mk all
	@cd unixsyms; $(MAKE) -f unixsyms.mk all $(MAKEARGS)
locklist.all:
	@echo $(MAKE) -f locklist.mk all
	@cd locklist; $(MAKE) -f locklist.mk all $(MAKEARGS)
ddicheck.all:
	@echo $(MAKE) -f ddicheck.mk all
	@cd ddicheck; $(MAKE) -f ddicheck.mk all $(MAKEARGS)

install:	idtools.install unixsyms.install locklist.install ddicheck.install

idtools.install:
	@echo $(MAKE) -f idtools.mk install
	@cd idtools; $(MAKE) -f idtools.mk install $(MAKEARGS)
unixsyms.install:
	@echo $(MAKE) -f unisxyms.mk install
	@cd unixsyms; $(MAKE) -f unixsyms.mk install $(MAKEARGS)
locklist.install:
	@echo $(MAKE) -f locklist.mk install
	@cd locklist; $(MAKE) -f locklist.mk install $(MAKEARGS)
ddicheck.install:
	@echo $(MAKE) -f ddicheck.mk install
	@cd ddicheck; $(MAKE) -f ddicheck.mk install $(MAKEARGS)

clean:	idtools.clean unixsyms.clean locklist.clean ddicheck.clean

idtools.clean:
	@echo $(MAKE) -f idtools.mk clean
	@cd idtools; $(MAKE) -f idtools.mk clean $(MAKEARGS)
unixsyms.clean:
	@echo $(MAKE) -f unisxyms.mk clean
	@cd unixsyms; $(MAKE) -f unixsyms.mk clean $(MAKEARGS)
locklist.clean:
	@echo $(MAKE) -f locklist.mk clean
	@cd locklist; $(MAKE) -f locklist.mk clean $(MAKEARGS)
ddicheck.clean:
	@echo $(MAKE) -f ddicheck.mk clean
	@cd ddicheck; $(MAKE) -f ddicheck.mk clean $(MAKEARGS)

clobber:	idtools.clobber unixsyms.clobber locklist.clobber ddicheck.clobber

idtools.clobber:
	@echo $(MAKE) -f idtools.mk clobber
	@cd idtools; $(MAKE) -f idtools.mk clobber $(MAKEARGS)
unixsyms.clobber:
	@echo $(MAKE) -f unisxyms.mk clobber
	@cd unixsyms; $(MAKE) -f unixsyms.mk clobber $(MAKEARGS)
locklist.clobber:
	@echo $(MAKE) -f locklist.mk clobber
	@cd locklist; $(MAKE) -f locklist.mk clobber $(MAKEARGS)
ddicheck.clobber:
	@echo $(MAKE) -f ddicheck.mk clobber
	@cd ddicheck; $(MAKE) -f ddicheck.mk clobber $(MAKEARGS)

lintit:	idtools.lintit unixsyms.lintit locklist.lintit

idtools.lintit:
	@echo $(MAKE) -f idtools.mk lintit
	@cd idtools; $(MAKE) -f idtools.mk lintit $(MAKEARGS)
unixsyms.lintit:
	@echo $(MAKE) -f unisxyms.mk lintit
	@cd unixsyms; $(MAKE) -f unixsyms.mk lintit $(MAKEARGS)
locklist.lintit:
	@echo $(MAKE) -f locklist.mk lintit
	@cd locklist; $(MAKE) -f locklist.mk lintit $(MAKEARGS)
