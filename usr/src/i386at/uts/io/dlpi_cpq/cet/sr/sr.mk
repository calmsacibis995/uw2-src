#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/sr/sr.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE	= sr.mk
KBASE		= ../../../..
LINTDIR		= $(KBASE)/lintdir
DIR		= io/dlpi_cpq/cet/common
LOCALDEF	= -DESMP
MOD		= nflxsr
CMDOWN		= root
CMDGRP		= sys

NFLXSR		= nflxsr.cf/Driver.o
LFILE		= $(LINTDIR)/nflxsr.ln

INSDIR		= $(ETC)/netflex
NFLXSRDIR		= nflxsr.cf

PROBEFILE	= nflxsr_hash.c
BINARIES	= $(NFLXSR)


NFLXSRFILES = \
	nflxsr_hash.o \
	nflxsr_str.o \
	nflxsr_wait.o

#CFILES used by depend.rules
CFILES = \
	nflxsr_hash.c \
	nflxsr_str.c \
	nflxsr_wait.c

LFILES = \
	nflxsr_hash.ln \
	nflxsr_str.ln \
	nflxsr_wait.ln

SRCFILES = \
	nflxsr_hash.c \
	nflxsr_str.c \
	nflxsr_wait.c \

.SUFFIXES: .ln

.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DESMP -c -u $*.c >> $*.L

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
	


DRIVER: $(NFLXSR)

install: all
	-[ -d $(INSDIR) ] || mkdir $(INSDIR)
	-[ -d $(ETC)/rc2.d ] || mkdir $(ETC)/rc2.d
	$(INS) -f $(ETC)/rc2.d -m 0644 -u $(CMDOWN) -g $(CMDGRP) $(NFLXSRDIR)/S02nflxt
	$(INS) -f $(INSDIR) -m 0644 -u $(CMDOWN) -g $(CMDGRP) $(NFLXSRDIR)/ap.nflxsr
	cd $(NFLXSRDIR); $(IDINSTALL) -R$(CONF) -M $(MOD)

$(NFLXSR): $(NFLXSRFILES)
	$(LD) -r -o $(NFLXSR) $(NFLXSRFILES)

clean:
	-rm -f *.o

clobber: clean
	-$(IDINSTALL) -R$(CONF) -e -d $(MOD)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

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
	nflxsr.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep

