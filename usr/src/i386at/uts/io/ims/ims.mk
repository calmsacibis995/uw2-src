#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/ims/ims.mk	1.3"

#***************************************************************************
#**
#**      MODULE NAME:  ims.mk
#**
#**      PURPOSE: io directory makefile for Tricord  
#**
#**      DEPENDENCIES:
#**
#**          o Tricord Powerframe Model 30/40 & ESxxxx hardware.
#**
#**      REVISION HISTORY:
#**      FPR/CRN     Date    Author      Description
#**
#**     
#****************************************************************************
#*/
include $(UTSRULES)

MAKEFILE = ims.mk
DIR = io/ims
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/ims.ln

LOCALINC = -I.

IMS = ims.cf/Driver.o

MODULES = $(IMS)
PROBEFILE = ims.c
BINARIES = $(IMS)

FILES = \
	ims.o \
	imsattn.o

CFILES = \
	ims.c \
	imsattn.c 

SRCFILES = $(CFILES) 

LFILES = \
	ims.ln \
	imsattn.ln

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
	cd ims.cf; $(IDINSTALL) -R$(CONF) -M ims 

binaries : $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $(IMS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L 

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e ims 
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
	ims.h \
	imsd.h \
	ims_mrp.h

sysHeaders = trimpic.h

headinstall:
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
