#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/cpqw/cpqw.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = cpqw.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/cpqw

CPQW = cpqw.cf/Driver.o
BINARIES = $(CPQW)
LFILE = $(LINTDIR)/cpqw.ln
PROBEFILE = cpqw.c

FILES= \
	cpqw.o \
	asr.o \
	ecc_nmi.o \
	cpqw_lib.o \
	csm.o \
	cpqw_rom_call.o

CFILES = \
	cpqw.c \
	asr.c \
	ecc_nmi.c \
	csm.c \
	cpqw_lib.c

SFILES = \
	cpqw_rom_call.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	asr.ln \
	cpqw.ln \
	csm.ln \
	ecc_nmi.ln \
	cpqw_lib.ln

SUBDIRS =

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

install:	all
		(cd cpqw.cf; $(IDINSTALL) -R$(CONF) -M cpqw)

binaries:	$(BINARIES)

$(BINARIES):	$(FILES)
		$(LD) -r -o $(CPQW) $(FILES)

cpqw_rom_call.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

touch:
		touch *.c

clean:
		-rm -f *.o $(LFILES) *.L
clobber:	clean
		-$(IDINSTALL) -R$(CONF) -d -e cpqw
		@if [ -f $(PROBEFILE) ]; then \
			echo "rm -f $(BINARIES)" ;\
			rm -f $(BINARIES) ;\
		fi


$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
		-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
		for i in $(LFILES); do \
				cat $$i >> $(LFILE); \
				cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln' `.L; \
		done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
		cpqw.h cpqw_cimpsw.h cpqw_lib.h

headinstall:
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
		if [ -f $$f ]; then \
		  $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
		fi ;\
	done

FRC:

include $(UTSDEPEND)
include $(MAKEFILE).dep
