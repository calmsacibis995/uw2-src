#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/netmgt.mk	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/netmgt.mk,v 1.3 1994/05/24 17:50:36 cyang Exp $"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(CMDRULES)

TOP = ..

include $(TOP)/local.def

SUBDIRS = mib \
	hostmibd \
	nwumps

all install clean clobber: 
	@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ $(MAKEARGS) ; cd .. ; \
	done

lintit: 
	-@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ ; cd .. ; \
	done
