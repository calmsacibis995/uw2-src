#ident	"@(#)mk:common/le/.le.mk	1.3"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


BASE=$(ROOT)/usr/src/$(WORK)
LEBASE=$(BASE)/le

INSTALL: $(LIST)

# MAKEFLAGS is set to NULL so -P is not passed.

$(LIST)::
	cd $(LEBASE)/$@; \
	echo "=== $@"; \
	MAKEFLAGS= $(MAKE) -f $@_le.mk install \
		 INSTROOT=$(ROOT)/$(MACH) >  $(BASE)/$@_le.out 2>&1
	cd $(LEBASE)/$@/build; \
	echo "=== $@"; \
	MAKEFLAGS= $(MAKE) -f boot.mk install >> $(BASE)/$@_le.out 2>&1
