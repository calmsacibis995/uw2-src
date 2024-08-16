#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#ident	"@(#)stand:i386at/standalone/boot/boot.mk	1.2.1.5"
#ident  "$Header: $"

include $(CMDRULES)

FPFTYPE =DEVDEF=-DSFBOOT	# DEVDEF=-DS5BOOT
ASFLAGS = -m
INITPROGDIR = $(ROOT)/$(MACH)/etc/initprog
INSDIR = $(ROOT)/$(MACH)/etc

# This will make the following:
#	wdboot (hard disk to hardisk boot strap)
#	hdboot (floppy to hardisk boot strap)
#	fdboot (floppy to floppy boot strap)
#	sip and mip
# The wdboot and the hdboot support only BFS file system type
# The fdboot support both s5 and BFS file sytem type.
# To make sip and mip, wdboot and/or hdboot must be build first
# So, to build all (i.e. wdboot, hdboot, fdboot, sip and mip)
# the boot libraries must be rebuild as well as some of the boot code.
# 
all: 	wdboot fdboot cpqfboot
	echo "**** Boot installation completed" > /dev/null

wdboot: bootlib
	cd at386; $(MAKE) -f at386.mk clobber $(MAKEARGS)
	cd bootlib; $(MAKE) -f bootlib.mk DEVDEF=-DWINI $(MAKEARGS)
	cd at386; $(MAKE) -f at386.mk $@ DEVDEF=-DWINI $(MAKEARGS)

fdboot: bootlib
	cd at386; $(MAKE) -f at386.mk clean $(MAKEARGS)
	cd bootlib; $(MAKE) -f bootlib.mk $(FPFTYPE) $(MAKEARGS)
	cd at386; $(MAKE) -f at386.mk $(FPFTYPE) $@  $(MAKEARGS)

hdboot: bootlib
	cd at386; $(MAKE) -f at386.mk clean $(MAKEARGS)
	cd bootlib; $(MAKE) -f bootlib.mk DEVDEF="-DWINI -DHDTST" $(MAKEARGS)
	cd at386; $(MAKE) -f at386.mk DEVDEF="-DWINI -DHDTST" $@ $(MAKEARGS)

cpqfboot: bootlib
	cd at386; $(MAKE) -f at386.mk clean $(MAKEARGS)
	cd bootlib; $(MAKE) -f bootlib.mk $(FPFTYPE) $(MAKEARGS)
	cd at386; $(MAKE) -f at386.mk $(FPFTYPE) $@  $(MAKEARGS)

bootlib: FRC
	cd bootlib; $(MAKE) -f bootlib.mk clobber $(MAKEARGS)

install: all
	-[ -d $(INITPROGDIR) ] || mkdir -p $(INITPROGDIR)
	cd at386; $(INS) -f $(INSDIR) -m 644 -u bin -g bin wdboot
	mv $(INSDIR)/wdboot $(INSDIR)/.wboot
	ln $(INSDIR)/.wboot $(INSDIR)/boot
	cd at386; $(INS) -f $(INSDIR) -m 644 -u bin -g bin fdboot
	mv $(INSDIR)/fdboot $(INSDIR)/.fboot
	ln $(INSDIR)/.fboot $(INSDIR)/fboot
	cd at386; $(INS) -f $(INSDIR) -m 644 -u bin -g bin cpqfboot
	cd at386/dcmp;$(INS) -f $(INITPROGDIR) -m 644 -u bin -g bin dcmp
	cd at386/sip;$(INS) -f $(INITPROGDIR) -m 644 -u bin -g bin sip
	cd at386/mip;$(INS) -f $(INITPROGDIR) -m 644 -u bin -g bin mip
	cd at386/mip;$(INS) -f $(INITPROGDIR) -m 644 -u bin -g bin coro_mip
	cd at386/tool;$(INS) -f $(INITPROGDIR) -m 644 -u bin -g bin logo.img

depend:: makedep
	@cd  bootlib;\
	echo "====== $(MAKE) -f bootlib.mk depend" ; \
	$(MAKE) -f bootlib.mk depend MAKEFILE=bootlib.mk $(MAKEARGS) 
	@echo "====== $(MAKE) -f at386.mk depend" ; \
	cd at386; $(MAKE) -f at386.mk depend MAKEFILE=at386.mk $(MAKEARGS) 

clean clobber :
	cd bootlib; $(MAKE) -f bootlib.mk $@ $(MAKEARGS)
	cd at386; $(MAKE) -f at386.mk $@ $(MAKEARGS)

FRC: 
