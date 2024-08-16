#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

BINDIR=$(ROOT)/$(MACH)/usr/vmsys/admin/PS
SUBDIR=CONFIG FILTER FORM WHEEL
BINARY=
SHELLS=
PRODUCTS=$(BINARY) $(SHELLS)

all:
	@echo "***** Making VMSYS/admin/PS"
	@echo "nothing to build."
	
install: all
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	-for f in $(SUBDIR) ; \
	do \
		( cd $$f ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done

clean clobber size strip lintit:
	-for f in $(SUBDIR) ; \
	do \
		( cd $$f ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done
