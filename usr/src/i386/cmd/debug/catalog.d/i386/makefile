#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:catalog.d/i386/makefile	1.14"

include $(CMDRULES)

AWK = awk
LIBINT = ../../libint
LIBCMD = ../../libcmd
DEBUGINC = ../../inc
GUI = ../../gui.d

include ../../util/common/std.make

include DEPEND
TARGET = $(OBJECTS)

all:	$(OBJECTS) $(HELPFILES)

install:	$(OBJECTS) $(HELPFILES)
	-if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES; \
	fi
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.lab.str
	cp Mcatalog $(USRLIB)/locale/C/MSGFILES/debug.str
	cp GMcatalog $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	cp GLcatalog $(USRLIB)/locale/C/MSGFILES/debug.lab.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.lab.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.help
	rm -f $(USRLIB)/locale/C/MSGFILES/dbg.help.thr
	cp $(COMMON)/cli.help $(USRLIB)/locale/C/MSGFILES/debug.help
	cp $(COMMON)/cli.help.thr $(USRLIB)/locale/C/MSGFILES/dbg.help.thr
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.help
	chmod 444 $(USRLIB)/locale/C/MSGFILES/dbg.help.thr

install_uw11:	install $(HELPFILES)
	-if [ ! -d $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug ]; then \
		mkdir -p $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug; \
	fi
	rm -f $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.hlp
	rm -f $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.defs
	cp $(HELPFILES) $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug
	chmod 444 $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/debug.*
