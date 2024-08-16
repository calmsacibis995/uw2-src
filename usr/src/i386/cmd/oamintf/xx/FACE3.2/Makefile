#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)
VMSYS=$(ROOT)/$(MACH)/usr/vmsys
MENUDIRS=OBJECTS HELP
CMDDIRS=bin admbin admin
FILES=README

all:
	@echo "======= Building oldface"
	-for dir in $(CMDDIRS) ; \
	do \
		echo "Making $$dir ..." ; \
		( cd $$dir ; $(MAKE) $(MAKEARGS) $@ ) ; \
	done

install: all
	@echo "======= Installing oldface"
	[ -d $(VMSYS) ] || mkdir -p $(VMSYS)
	$(INS) -m 644 -g bin -u bin -f $(VMSYS) README

	for dir in $(MENUDIRS) $(CMDDIRS) ; \
	do \
		echo "Installing $$dir ..." ; \
		( cd $$dir ; $(MAKE) $(MAKEARGS) $@ ) ; \
	done

clean clobber size strip lintit:
	@echo "======= Cleaning/Clobbering oldface"
	-for dir in $(MENUDIRS) $(CMDDIRS) ; \
	do \
		echo "Cleaning/Clobbering $$dir ..." ; \
		( cd $$dir ; $(MAKE) $@ ) ; \
	done
