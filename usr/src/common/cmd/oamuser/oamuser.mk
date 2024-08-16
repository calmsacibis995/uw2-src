#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamuser:oamuser.mk	1.8.9.2"
#ident  "$Header: oamuser.mk 2.1 91/07/26 $"

include $(CMDRULES)

DIRS = lib group user

all clean clobber lintit size strip:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd .. ; \
	done

install :  all
	-[ -d $(USRSADM) ] || mkdir -p $(USRSADM)
	-[ -d $(ETC)/skel ] || mkdir -p $(ETC)/skel
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd .. ; \
	done
