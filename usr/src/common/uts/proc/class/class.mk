#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:proc/class/class.mk	1.14"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	class.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = proc/class

SYSCLASS = sysclass.cf/Driver.o
TS = ts.cf/Driver.o
FP = fp.cf/Driver.o
FC=fc.cf/Driver.o
LFILE = $(LINTDIR)/class.ln

MODULES = \
	$(SYSCLASS) \
	$(TS) \
	$(FP) \
	$(FC)

LFILES = \
	sysclass.ln \
	fp.ln \
	ts.ln \
	fc.ln

CFILES = sysclass.c \
	 fp.c \
	 ts.c \
	 fc.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	cd sysclass.cf; $(IDINSTALL) -R$(CONF) -M sysclass
	cd ts.cf; $(IDINSTALL) -R$(CONF) -M ts
	cd fp.cf; $(IDINSTALL) -R$(CONF) -M fp
	cd fc.cf; $(IDINSTALL) -R$(CONF) -M fc

$(SYSCLASS):	sysclass.o
	$(LD) -r -o $(SYSCLASS) sysclass.o

$(TS):	ts.o
	$(LD) -r -o $(TS) ts.o

$(FP):	fp.o
	$(LD) -r -o $(FP) fp.o

$(FC):	fc.o
	$(LD) -r -o $(FC) fc.o

clean:
	-rm -f sysclass.o ts.o fp.o fc.o $(MODULES) $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e sysclass
	-$(IDINSTALL) -R$(CONF) -d -e ts
	-$(IDINSTALL) -R$(CONF) -d -e fp
	-$(IDINSTALL) -R$(CONF) -d -e fc

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
	tspriocntl.h \
	fppriocntl.h \
	fcpriocntl.h \
	rtpriocntl.h \
	rt.h \
	ts.h \
	fpri.h \
	fc.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
