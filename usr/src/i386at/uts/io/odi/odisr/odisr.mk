#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/odisr/odisr.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE= odisr.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/odisr
MOD     = odisr.cf/Driver.o
LFILE   = $(LINTDIR)/odisr.ln
BINARIES = $(MOD)
LOCALDEF = -DDL_STRLOG
PROBEFILE = srsup.c

FILES = srsup.o \
	sr_wrap.o 

CFILES = srsup.c \
	sr_wrap.c

LFILES = srsup.ln \
	sr_wrap.ln

SRCFILES = $(CFILES)

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
	cd odisr.cf; $(IDINSTALL) -R$(CONF) -M odisr

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d odisr
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

headinstall: route.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) route.h

fnames:
	@for i in $(FILES);  \
	do                      \
		echo $$i;       \
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

lintit:
 
include $(UTSDEPEND)

include $(MAKEFILE).dep
