#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/security/audit/Makefile	1.1.7.3"
#ident "$Header: Makefile 2.0 91/07/12 $"
# Makefile for OAM Security Module: Auditing Files

include $(CMDRULES)

DIRS=bin

MI_FILES=audit.mi

AUD_FILES=\
	Help \
	auditing/Help \
	auditing/parameters/Help \
	auditing/parameters/set/Text.setok \
	auditing/parameters/set/Help \
	auditing/parameters/set/Form.log \
	auditing/parameters/set/Text.setnok \
	auditing/parameters/set/Text.setwok \
	auditing/parameters/set/Form.set \
	auditing/parameters/list/Help \
	auditing/parameters/list/Text.paramlst \
	auditing/report/Form.rep2 \
	auditing/report/Form.report \
	auditing/report/Help \
	auditing/report/Menu.events \
	auditing/report/Menu.logfs \
	auditing/report/Menu.obtypes \
	auditing/report/Menu.privs \
	auditing/report/Text.repnok \
	auditing/report/Text.repwok \
	auditing/report/Text.repok \
	auditing/criteria/Help \
	auditing/criteria/set/Help \
	auditing/criteria/set/object/Form.objaud \
	auditing/criteria/set/object/Text.objok \
	auditing/criteria/set/object/Text.objwok \
	auditing/criteria/set/object/Help \
	auditing/criteria/set/object/Menu.events \
	auditing/criteria/set/object/Text.objnok \
	auditing/criteria/set/system/Form.sysaud \
	auditing/criteria/set/system/Text.sysok \
	auditing/criteria/set/system/Help \
	auditing/criteria/set/system/Text.sysnok \
	auditing/criteria/set/system/Text.syswok \
	auditing/criteria/set/system/Menu.events \
	auditing/criteria/set/user/Menu.logins \
	auditing/criteria/set/user/Text.userok \
	auditing/criteria/set/user/Help \
	auditing/criteria/set/user/Menu.events \
	auditing/criteria/set/user/Form.useraud \
	auditing/criteria/set/user/Text.usernok \
	auditing/criteria/set/user/Text.userwok \
	auditing/criteria/list/Text.audlst \
	auditing/criteria/list/Help \
	auditing/enable/Form.enable \
	auditing/enable/Text.onnok \
	auditing/enable/Text.onwok \
	auditing/enable/Text.onok \
	auditing/enable/Help \
	auditing/disable/Form.disable \
	auditing/disable/Help \
	auditing/disable/Text.offok \
	auditing/disable/Text.offnok \
	auditing/disable/Text.offwok

all:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

lintit clobber size strip clean:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

install: all xinstall
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

PROTO=Prototype
OBJECTS=$(MI_FILES) $(AUD_FILES)
include ../../Install
