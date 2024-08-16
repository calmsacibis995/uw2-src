#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/lsl/lsl.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE= lsl.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/lsl
MOD     = lsl.cf/Driver.o
LFILE   = $(LINTDIR)/lsl.ln
BINARIES = $(MOD)

LOCALDEF = -DDL_STRLOG 
PROBEFILE = lslodi.c

FILES	= lslodi.o \
	  lslwrap.o \
	  lslstr.o \
	  lslsubr.o \
	  lslxmorg.o

LFILES	= lslodi.ln \
	  lslwrap.ln \
	  lslstr.ln \
	  lslsubr.ln \
	  lslxmorg.ln

CFILES	= lslodi.c \
	  lslwrap.c  \
	  lslstr.c \
	  lslsubr.c \
	  lslxmorg.c

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

install:all
	(cd lsl.cf; $(IDINSTALL) -R$(CONF) -M lsl)


clean:
	-rm -f *.o $(LFILES) *.L

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d lsl
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
		cat `basename $$i .ln`.L >> `expr $(LFILE) :    \
			'\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do                      \
		echo $$i;       \
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

lslHeaders = \
        lsl.h

headinstall:$(lslHeaders)
	@for f in $(lslHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
