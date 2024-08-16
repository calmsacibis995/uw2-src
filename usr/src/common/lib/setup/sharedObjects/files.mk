#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsetup:sharedObjects/files.mk	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/setup/sharedObjects/files.mk,v 1.11 1994/04/04 14:43:43 sharriso Exp $"

include $(LIBRULES)

all clean clobber lintit:
	cd mailcnfg; make -f *.mk $@
	cd mailflgs; make -f *.mk $@
	cd smfcnfg; make -f *.mk $@

install: $(USRLIB)/setup/filetypes
	cd mailcnfg; make -f *.mk $@
	cd mailflgs; make -f *.mk $@
	cd smfcnfg; make -f *.mk $@
	$(INS) -f $(USRLIB)/setup/filetypes -m 644 config

localinstall: /usr/lib/setup/filetypes
	cd mailcnfg; make -f *.mk $@
	cd mailflgs; make -f *.mk $@
	cd smfcnfg; make -f *.mk $@
	$(INS) -f /usr/lib/setup/filetypes -m 644 config

/usr/lib/setup /usr/lib/setup/filetypes $(USRLIB)/setup/filetypes $(USRLIB)/setup:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp bin $@
	$(CH)chmod 755 $@
