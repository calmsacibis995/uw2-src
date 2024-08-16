#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:cpq.cmds.mk	1.9"

include $(CMDRULES)

DIRS = snmp eisautil wellness ups nic inspect cpqsmu cpqscsimon ida_menu cpqidamon

all clobber install clean strip lintit:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f $$i.mk $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f $$i.mk $@ ;\
		 	cd .. ;\
		fi ;\
	done
