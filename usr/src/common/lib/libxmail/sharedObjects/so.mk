#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmail:sharedObjects/so.mk	1.9"
include $(LIBRULES)
USRLIB=$(ROOT)/$(MACH)/usr/lib

all clean clobber lintit:
		cd dns; $(MAKE) -f *.mk $@
		cd file; $(MAKE) -f *.mk $@
		cd home; $(MAKE) -f *.mk $@
		cd nis; $(MAKE) -f *.mk $@
		cd passwd; $(MAKE) -f *.mk $@

install: $(USRLIB)/mail/libalias
		cd dns; $(MAKE) -f *.mk $@
		cd file; $(MAKE) -f *.mk $@
		cd home; $(MAKE) -f *.mk $@
		cd nis; $(MAKE) -f *.mk $@
		cd passwd; $(MAKE) -f *.mk $@
		$(INS) -f $(LIB)/mail -m 644 lookupLibs.proto

localinstall: /usr/lib/mail/libalias
		cd dns; $(MAKE) -f *.mk $@
		cd file; $(MAKE) -f *.mk $@
		cd home; $(MAKE) -f *.mk $@
		cd nis; $(MAKE) -f *.mk $@
		cd passwd; $(MAKE) -f *.mk $@
		$(INS) -f /usr/lib/mail -m 644 lookupLibs.proto

$(USRLIB)/mail/libalias:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp mail $@

/usr/lib/mail/libalias:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp mail $@
