#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)messages:common/cmd/messages/nis.pkg/msgs.mk	1.2"
#ident  "$Header: $"

include $(CMDRULES)

PKG=nis
CATALOG=$(PKG).pkg.str
MSGDIR=$(USRLIB)/locale/C/MSGFILES

all	: msgs

install: all 
	cp msgs $(CATALOG)
	[ -d $(MSGDIR) ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 $(CATALOG)

lintit : 

clean :
	rm -f $(CATALOG)

clobber : clean

