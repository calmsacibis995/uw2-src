#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:lprof.mk	1.10.1.13"

include $(CMDRULES)

include lprofinc.mk

all:  basicblk cmds 

basicblk:
	cd bblk/$(CPU); $(MAKE) -f bblk.mk

cmds:
	if test "$(NATIVE)" = "yes"; then \
		cd cmd; $(MAKE) -f cmd.mk ; \
	fi

install: all
	$(MAKE) target -f lprof.mk LPTARGET=install

lintit:
	$(MAKE) target -f lprof.mk LPTARGET=lintit

clean:
	$(MAKE) target -f lprof.mk LPTARGET=clean

clobber: clean
	$(MAKE) target -f lprof.mk LPTARGET=clobber

target:
	cd bblk/$(CPU); \
	$(MAKE) -f bblk.mk $(LPTARGET); \
	cd ../..; 
	if test "$(NATIVE)" = "yes"; then \
	   cd cmd; \
	   $(MAKE) -f cmd.mk $(LPTARGET) ; \
	   cd ..; \
	   cd libprof/$(CPU); \
	   $(MAKE) -f libprof.mk $(LPTARGET) ; \
	   cd ../..; \
	fi
