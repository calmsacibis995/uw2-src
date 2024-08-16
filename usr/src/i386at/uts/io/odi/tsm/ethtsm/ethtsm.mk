#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/tsm/ethtsm/ethtsm.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE= ethtsm.mk
KBASE	= ../../../..
LINTDIR = $(KBASE)/lintdir
DIR	= io/tsm/ethtsm
MOD	= ethtsm.cf/Driver.o
LFILE	= $(LINTDIR)/ethtsm.ln
BINARIES = $(MOD)
PROBEFILE = ethertsm.c

#DEBUGDEF = -DDEBUG_TRACE -DNVLT_ModMask=NVLTM_odi
#CC	= epicc -W0,-2N -W0,"-M 0x00020100"	# ODI mask
LOCALDEF = -DODI_3_0 $(DEBUGDEF)
CFLAGS	= -O -I$(ODIINC) $(LOCALDEF)

FILES	= \
	ethwrap.o \
	ethertsm.o \
	etherglu.o

LFILES 	= \
	ethwrap.ln \
	ethertsm.ln \
	etherglu.ln

CFILES 	= \
	ethwrap.c \
	ethertsm.c

SRCFILES= $(CFILES)

SFILES 	= \
	etherglu.s

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

install:all
	(cd ethtsm.cf; $(IDINSTALL) -R$(CONF) -M ethtsm)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d ethtsm
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkaux -p $@

lintit: $(LFILE)

$(LFILE):$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) :	\
				'\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do			\
		echo $$i;	\
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

ethtsmHeaders = \
	ethdef.h

headinstall: $(ethtsmHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(ethtsmHeaders);	\
	do				\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
