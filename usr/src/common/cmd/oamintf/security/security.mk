#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/security/security.mk	1.1.6.2"
#ident "$Header: security.mk 2.0 91/07/12 $"

include $(CMDRULES)

DIRS=audit es

all:
	@for i in $(DIRS) ;\
	do \
		if [ -d $$i ] ;\
		then \
			echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
			if cd $$i ;\
			then \
				$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
				cd .. ;\
			fi ;\
		fi ;\
	done

lintit clobber size strip clean:
	@for i in $(DIRS) ;\
	do \
		if [ -d $$i ] ;\
		then \
			echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
			if cd $$i ;\
			then \
				$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
				cd .. ;\
			fi ;\
		fi ;\
	done

install: all
	@for i in $(DIRS) ;\
	do \
		if [ -d $$i ] ;\
		then \
			echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
			if cd $$i ;\
			then \
				$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
				cd .. ;\
			fi ;\
		fi ;\
	done

