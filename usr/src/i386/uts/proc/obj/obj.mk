#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:proc/obj/obj.mk	1.20"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	obj.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = proc/obj

DOSX = dosx.cf/Driver.o
XOUT = xout.cf/Driver.o
COFF = coff.cf/Driver.o
ELF = elf.cf/Driver.o
INTP = intp.cf/Driver.o
I286X = i286x.cf/Driver.o
COFF_STUB = coff.cf/Modstub.o
LFILE = $(LINTDIR)/obj.ln

MODULES = \
	$(DOSX) \
	$(XOUT) \
	$(COFF) \
	$(ELF) \
	$(I286X) \
	$(INTP)

MODSTUBS = \
	$(COFF_STUB)

DOSX_FILES = dosx.o
XOUT_FILES = xout.o
COFF_FILES = coff.o
ELF_FILES = elf.o
I286X_FILES = i286x.o
INTP_FILES = intp.o

COFF_STUB_FILES = coff_stub.o

FILES = \
	$(DOSX_FILES) \
	$(XOUT_FILES) \
	$(COFF_FILES) \
	$(ELF_FILES) \
	$(I286X_FILES) \
	$(INTP_FILES)

LFILES = \
	dosx.ln \
	xout.ln \
	coff.ln \
	elf.ln \
	i286x.ln \
	intp.ln

CFILES = \
	dosx.c \
	xout.c \
	coff.c \
	elf.c \
	i286x.c \
	intp.c

SRCFILES = $(CFILES)

all:	$(MODULES) $(MODSTUBS)

install: all
	(cd dosx.cf; $(IDINSTALL) -R$(CONF) -M dosx)
	(cd xout.cf; $(IDINSTALL) -R$(CONF) -M xout)
	(cd coff.cf; $(IDINSTALL) -R$(CONF) -M coff)
	(cd elf.cf; $(IDINSTALL) -R$(CONF) -M elf)
	(cd i286x.cf; $(IDINSTALL) -R$(CONF) -M i286x)
	(cd intp.cf; $(IDINSTALL) -R$(CONF) -M intp)

$(DOSX): $(DOSX_FILES)
	$(LD) -r -o $(DOSX) $(DOSX_FILES)

$(XOUT): $(XOUT_FILES)
	$(LD) -r -o $(XOUT) $(XOUT_FILES)

$(COFF): $(COFF_FILES)
	$(LD) -r -o $(COFF) $(COFF_FILES)

$(ELF): $(ELF_FILES)
	$(LD) -r -o $(ELF) $(ELF_FILES)

$(I286X): $(I286X_FILES)
	$(LD) -r -o $(I286X) $(I286X_FILES)

$(INTP): $(INTP_FILES)
	$(LD) -r -o $(INTP) $(INTP_FILES)

$(COFF_STUB): $(COFF_STUB_FILES)
	$(LD) -r -o $(COFF_STUB) $(COFF_STUB_FILES)

clean:
	-rm -f *.o $(LFILES) $(LFILE) *.L $(DOSX) $(XOUT) $(COFF) $(ELF) $(INTP) $(I286X) $(COFF_STUB)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e dosx
	-$(IDINSTALL) -R$(CONF) -d -e xout
	-$(IDINSTALL) -R$(CONF) -d -e coff
	-$(IDINSTALL) -R$(CONF) -d -e elf
	-$(IDINSTALL) -R$(CONF) -d -e i286x
	-$(IDINSTALL) -R$(CONF) -d -e intp

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	elf.h \
	elf_386.h \
	elftypes.h \
	x.out.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
