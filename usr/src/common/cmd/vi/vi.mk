#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)vi:vi.mk	1.33.1.4"
#ident  "$Header: vi.mk 1.5 91/07/01 $"

include $(CMDRULES)

#	Makefile for vi

OWN = root
GRP = sys

MKDIR = port
MKFILE = makefile.usg     

all : 
	@echo "\n\t>Making commands."
	cd misc; $(MAKE) $(MAKEARGS) all; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) all ; cd ..
	@echo "Finished compiling..."

install: all
	if [ ! -d $(ETC)/init.d ] ; then mkdir $(ETC)/init.d ; fi
	if [ ! -d $(ETC)/rc2.d ] ; then mkdir $(ETC)/rc2.d ; fi
	cd misc; $(MAKE) $(MAKEARGS) install
	@echo "\n\t> Installing ex object."
	cd $(MKDIR) ; $(MAKE) -f $(MKFILE) $(MAKEARGS) install
	@echo "\n\t> Creating PRESERVE scripts."
	 $(INS) -f $(ETC)/init.d -m 0444 -u $(OWN) -g $(GRP) PRESERVE
	-rm -f $(ETC)/rc2.d/S02PRESERVE
	-ln -f $(ETC)/init.d/PRESERVE $(ETC)/rc2.d/S02PRESERVE

#
# Cleanup procedures
#
clean lintit:
	cd misc; $(MAKE) $(MAKEARGS) $@ ; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) $@ ; cd ..

clobber: clean
	cd misc; $(MAKE) $(MAKEARGS) clobber ; cd ..
	cd $(MKDIR); $(MAKE) -f $(MKFILE) $(MAKEARGS) clobber ; cd ..

#	These targets are useful but optional

partslist productdir product srcaudit:
	cd misc ; $(MAKE) $(MAKEARGS) $@ ; cd ..
	cd $(MKDIR) ; $(MAKE) -f $(MKFILE) $(MAKEARGS) $@ ; cd ..
