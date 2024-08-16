#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:top.mk	1.4"

include $(CMDRULES)

DIRS = include libadm libcmd libpkg oampkg

all:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -P $(MAKEARGS) $@ ;\
		 	cd .. ;\
		fi ;\
	done

