#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)acp:i386/cmd/acp/acp.mk	1.2.1.6"

include	$(CMDRULES)

LDFLAGS=-lgen
COMMANDS = fsck mount
FRC =
EVENT = $(USRLIB)/event

all:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) all); \
	done

install: $(EVENT)
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) install); \
	done
	$(INS) -f $(EVENT) -m 644 -u bin -g bin devices
	$(INS) -f $(EVENT) -m 644 -u bin -g bin ttys
	$(INS) -f $(ETC) -m 644 -u bin -g bin socket.conf

$(EVENT):
	-mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp sys $@
	$(CH)chown root $@

clean:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk clean); \
	done

clobber:	clean
