#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nps.mk	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nps/nps.mk,v 1.5 1994/02/14 16:55:51 vtag Exp $"

include $(CMDRULES)

TOP = ..

include $(TOP)/local.def

SUBDIRS = \
	diagd \
	npsd \
	nvt \
	nwdiscover \
	sapd \
	utils

all install clean clobber: 
	@for i in $(SUBDIRS) ; \
	do \
	(echo "	cd $$i" ; \
	cd $$i ; \
	echo "	$(MAKE) -f $$i.mk $@ $(MAKEARGS)" ; \
	$(MAKE) -f $$i.mk $@ $(MAKEARGS)) ; \
	done

lintit: 
	-@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $@ ; cd .. ; \
	done
