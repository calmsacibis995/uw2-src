#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/tricord/tricord.mk	1.10"
#ident	"$Header: $"

#*
#** ident @(#) tricord.mk 1.2 1 2/11/94 11:49:06
#**
#** sccs_id[] = {"@(#) 1.2 tricord.mk "}
#*/

#*
#***************************************************************************
#**
#**      MODULE NAME:  tricord.mk
#**
#**      PURPOSE: svc directory makefile for Tricord  
#**
#**      DEPENDENCIES:
#**
#**          o Tricord Powerframe Model 30/40 & ESxxxx hardware.
#**
#**      REVISION HISTORY:
#**      FPR/CRN     Date    Author      Description
#**
#**		2/4/93      M. Conner   Initial dev. TLP5/x27 rel.
#**     
#****************************************************************************
#*/
include $(UTSRULES)

MAKEFILE = tricord.mk
DIR = psm/tricord
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/tricord.ln

LOCALINC = -I.

TRICORD = tricord.cf/Driver.o

MODULES = $(TRICORD)
PROBEFILE = tricord.c
BINARIES = $(TRICORD)

FILES = \
	tricord.o \
	tricord_phys.o \
	apic.o \
	apit.o \
	clockintr.o \
	intr_p.o \
	pic.o \
	spl.o

CFILES = \
	tricord.c \
	apic.c \
	apit.c \
	clockintr.c \
	pic.c

SFILES = \
	tricord_phys.s \
	intr_p.s \
	spl.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	tricord.ln \
	apic.ln \
	apit.ln \
	clockintr.ln \
	pic.ln

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
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



install: all
	cd tricord.cf; $(IDINSTALL) -R$(CONF) -M tricord 

binaries : $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $(TRICORD) $(FILES)

tricord_phys.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L 

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e tricord 
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi


lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

Headers = \
	triccs.h

sysHeaders = \
	triebs.h \
	trimms.h \
	triss.h

headinstall: $(Headers)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(Headers); \
	do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done
	@if [ -f $(PROBEFILE) ]; then \
		for f in $(sysHeaders); \
		do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
		done; \
	fi



include $(UTSDEPEND)

include $(MAKEFILE).dep
