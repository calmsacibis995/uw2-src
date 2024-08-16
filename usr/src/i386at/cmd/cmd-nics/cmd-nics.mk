#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nics:cmd-nics.mk	1.6"

#
#  These are the winxksh help files that are pulled into the nics
#  package.  They do not get installed into the $ROOT/$MACH tree
#  since the nics package prototype file needs to find them under
#  the $ROOT/usr/src/$WORK/cmd/cmd-nics directory.  'hcomp' is a
#  command that gets built from $ROOT/usr/src/$WORK/cmd/ahcomp.
#
include $(CMDRULES)

SUBDIRS = help supported_nics

all:
	$(CC) -o tools/findvt tools/findvt.c
	for d in $(SUBDIRS) ; \
	do \
		cd $$d; \
		for i in *; \
		do \
			[ -f $$i ] && $(USRBIN)/hcomp $$i ; \
		done ; \
		cd .. ; \
	done

install: all

clean:
	for d in $(SUBDIRS) ; \
	do \
		cd $$d; \
		$(RM) -rf *.hcf
		cd ..; \
	done

clobber: clean
