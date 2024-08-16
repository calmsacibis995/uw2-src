#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)messages:common/cmd/messages/netmgt.pkg/msgs.mk	1.1"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/messages/netmgt.pkg/msgs.mk,v 1.1 1994/08/04 16:02:30 cyang Exp $"

include $(CMDRULES)

PKG=netmgt
CATALOG=$(PKG).pkg.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 644 $(CATALOG)

clean :
	rm -f $(CATALOG)


