#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:config.d/i386/makefile	1.1"

include $(CMDRULES)

include ../../util/common/std.make

CONFFILES= \
	../common/config.bot \
	../common/config.cmd \
	../common/config.ctxt \
	../common/config.proc \
	../common/config.sep \
	../common/config.syms

all:	$(CONFFILES)

install:	$(CONFFILES)
	if [ ! -d $(CCSLIB)/config ]; then \
		mkdir $(CCSLIB)/config; \
	fi
	rm -rf $(CCSLIB)/config/*
	cp $(CONFFILES) $(CCSLIB)/config
	chmod 444 $(CCSLIB)/config/*
