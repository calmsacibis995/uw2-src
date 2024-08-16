#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/ether/ether.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE	= ether.mk
KBASE		= ../../../..
LINTDIR		= $(KBASE)/lintdir
DIR		= io/dlpi_cpq/cet/common
LOCALDEF	= -DETHER -DNFLXE -DESMP
LOCALINC	= -I$(ROOT)/$(MACH)/usr/include
DRV		= nflxe
CMDOWN		= root
CMDGRP		= sys

NFLXE		= nflxe.cf/Driver.o
LFILE		= $(LINTDIR)/nflxe.ln

INSDIR		= $(ETC)/netflex
COMMON		= ../common
DLPI_ETHER	= $(KBASE)/io/dlpi_ether
NETFLXECFDIR	= nflxe.cf

PROBEFILE	= $(COMMON)/cet.c
BINARIES	= $(NFLXE)


CETCMDS	= cet_start cet_stop

CETFILES = \
	dlpi_nflxe.o \
	cet.o \
	cet_lli.o \
	cet_mac.o

#CFILES for depend.rules
CFILES = \
	$(DLPI_ETHER)/dlpi_ether.c \
	$(COMMON)/cet.c \
	$(COMMON)/cet_lli.c \
	$(COMMON)/cet_mac.c

LOCLINKCFILES = \
	dlpi_ether.c \
	cet.c \
	cet_lli.c \
	cet_mac.c

LFILES = \
	cet.ln \
	cet_lli.ln \
	cet_mac.ln \
	dlpi_ether.ln

SRCFILES = \
	$(COMMON)/cet.c \
	$(COMMON)/cet_lli.c \
	$(COMMON)/cet_mac.c \
	$(DLPI_ETHER)/dlpi_ether.c

.SUFFIXES: .ln

.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DETHER -DNFLXE -DESMP -c -u $*.c >> $*.L

all:	
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi
	


DRIVER: $(NFLXE) $(CETCMDS)

install: all
	-[ -d $(INSDIR) ] || mkdir $(INSDIR)
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) cet_start
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) cet_stop
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) $(NETFLXECFDIR)/unieth.bin
	$(INS) -f $(INSDIR) -m 0755 -u $(CMDOWN) -g $(CMDGRP) $(NETFLXECFDIR)/uniethf.bin
	-[ -d $(ETC)/rc2.d ] || mkdir $(ETC)/rc2.d
	$(INS) -f $(ETC)/rc2.d -m 0644 -u $(CMDOWN) -g $(CMDGRP) $(NETFLXECFDIR)/S02nflxe
	cd $(NETFLXECFDIR); $(IDINSTALL) -R$(CONF) -M $(DRV)

$(NFLXE): $(CETFILES)
	$(LD) -r -o $(NFLXE) $(CETFILES)

cet_start:	$(COMMON)/cet_start.c
		$(CC) -o cet_start $(INCLIST) $(LOCALINC) $(LOCALDEF) -U_KERNEL_HEADERS $(COMMON)/cet_start.c 

cet_stop:	$(COMMON)/cet_stop.c
		$(CC) -o cet_stop  $(INCLIST) $(LOCALINC) $(LOCALDEF) -U_KERNEL_HEADERS  $(COMMON)/cet_stop.c

clean:
	-rm -f *.o

clobber: clean
	-$(IDINSTALL) -R$(CONF) -e -d $(DRV)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES) $(CETCMDS)" ;\
		rm -f $(BINARIES) ;\
		rm -f $(CETCMDS) ;\
	fi

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)
	-rm -f dlpi_nflxe.c

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

#
# Header Install Section
#

sysHeaders = \
	dlpi_$(DRV).h \
	$(COMMON)/cet.h

HEADERS = $(COMMON)/net.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@if [ -f $(PROBEFILE) ]; then \
		for f in $(HEADERS); \
	 	do \
	    	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 	done; \
	fi

FRC:

dlpi_nflxe.o: dlpi_ether.o
	cp dlpi_ether.o dlpi_nflxe.o

##########
#Makeing symbolic links in this directory of source files residing outside of
#makefile's directory in order avoid problem with make's inference rules
#which do not expand $< from files that reside outside of makefile's directory.
#
#Placement of this depend must be before including UTSDEPEND since we want this
#depend to execute first and build symbolic link files.
#
depend:: $(LOCLINKCFILES)

$(LOCLINKCFILES):
	for f in $(CFILES); \
	do \
		if [ "`basename $$f`" = "$@" ] ; \
		then \
			ln -s  $$f $@; \
		fi \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep

