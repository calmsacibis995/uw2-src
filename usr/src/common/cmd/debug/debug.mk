#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:debug.mk	1.5"

# The "all" target builds both the command line interface
# and the graphical user interface"
# The "cli" and "gui" targets build one or the other

include $(CMDRULES)
include util/common/defs.make

all:
	$(MAKE) -f debugsrc.mk all $(MAKEARGS) MACHDEFS=threads.defs

cli:
	$(MAKE) -f debugsrc.mk cli $(MAKEARGS) MACHDEFS=threads.defs

gui:
	$(MAKE) -f debugsrc.mk gui $(MAKEARGS) MACHDEFS=threads.defs

install:
	$(MAKE) -f debugsrc.mk install $(MAKEARGS) MACHDEFS=threads.defs

install_cli:
	$(MAKE) -f debugsrc.mk install_cli $(MAKEARGS) MACHDEFS=threads.defs

install_gui:
	$(MAKE) -f debugsrc.mk install_gui $(MAKEARGS) MACHDEFS=threads.defs

all_uw11:
	$(MAKE) -f debugsrc.mk all $(MAKEARGS)

cli_uw11:
	$(MAKE) -f debugsrc.mk cli $(MAKEARGS)

gui_uw11:
	$(MAKE) -f debugsrc.mk gui $(MAKEARGS)

install_all_uw11:
	$(MAKE) -f debugsrc.mk install_all_uw11 $(MAKEARGS)

install_cli_uw11:
	$(MAKE) -f debugsrc.mk install_cli_uw11 $(MAKEARGS)

install_gui_uw11:
	$(MAKE) -f debugsrc.mk install_gui_uw11 $(MAKEARGS)

lintit:
	$(MAKE) -f debugsrc.mk lintit $(MAKEARGS)

clean:
	$(MAKE) -f debugsrc.mk clean $(MAKEARGS)

clean_cli:
	$(MAKE) -f debugsrc.mk clean_cli $(MAKEARGS)

clean_gui:
	$(MAKE) -f debugsrc.mk clean_gui $(MAKEARGS)

clobber:
	$(MAKE) -f debugsrc.mk clobber $(MAKEARGS)

