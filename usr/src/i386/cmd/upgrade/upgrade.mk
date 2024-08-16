#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/upgrade.mk	1.18"
#ident	"$Header:$"

# Makefile for the component upgrade
include $(CMDRULES)

UPDIR = $(ETC)/inst
INSDIR1 = $(UPDIR)/save
INSDIR2 = $(USRSBIN)/pkginst
INSDIR3 = $(ETC)/inst/locale/C/menus/upgrade
INSDIR4 = $(USRLIB)/locale/C/MSGFILES

DIRS =  diffs  tools msgs

# list all the shell scripts in the tools directory without the suffix ".sh" 
# even if the name of the script has suffix .sh.

SCRIPTS = up_merge updebug chkpkgrel pkgmrgconf pkgsavfiles chkmrgfiles olscripts pkgrem up_cleanup

BASE = base.v4.log boot.inst straglers.v4

all: 
	: do nothing

install:	 $(DIRS)
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	[ -d $(INSDIR3) ] || mkdir -p $(INSDIR3)
	[ -d $(INSDIR4) ] || mkdir -p $(INSDIR4)
	[ -d $(UPDIR) ] || mkdir -p $(UPDIR)
	$(INS) -f $(INSDIR4) -m 0644 -u bin -g bin uxupgrade.str
	cd diffs; find . -follow -print | cpio -pcvduL $(INSDIR1)
	cd tools;\
	for i in $(SCRIPTS) ; \
	do \
		cp $$i.sh $$i ; \
		$(CH)chmod 755 $$i ; \
		$(INS) -f $(INSDIR2) -m 0755 -u bin -g bin $$i; \
	done ; 
	cd msgs; find . -follow -print | cpio -pcvduL $(INSDIR3)
	cd tools;\
	for i in $(BASE) ; \
	do \
		$(INS) -f $(UPDIR) -m 0755 -u bin -g bin $$i; \
	done;

clean:
	cd tools;\
	for i in $(SCRIPTS) ;\
	do \
		rm -f  $$i ;\
	done
	
	: do nothing

clobber:
	: do nothing

