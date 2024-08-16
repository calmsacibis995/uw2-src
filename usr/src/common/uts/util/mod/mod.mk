#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:util/mod/mod.mk	1.14"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mod.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = util/mod

INSPERM = -m $(INCMODE) -u $(OWN) -g $(GRP)
MOD = mod.cf/Driver.o
MODKSYM = modksym.cf/Driver.o
LFILE = $(LINTDIR)/mod.ln

FILES = \
	mod_drv.o \
	mod_intr.o \
	mod_str.o \
	mod_fs.o \
	modctl.o \
	modpath.o \
	modadm.o \
 	mod_obj.o \
	mod_objmd.o \
	mod_exec.o \
	mod_misc.o

CFILES = \
	mod_drv.c \
	mod_intr.c \
	mod_str.c \
	mod_fs.c \
	modadm.c \
	mod_obj.c \
	mod_objmd.c \
	mod_misc.c \
	modctl.c \
	modpath.c \
	modinit.c \
	mod_exec.c \
	mod_ksym.c

SRCFILES = $(CFILES)

LFILES = \
	mod_drv.ln \
	mod_intr.ln \
	mod_str.ln \
	mod_fs.ln \
	modadm.ln \
	mod_objmd.ln \
	mod_obj.ln \
	mod_misc.ln \
	modctl.ln \
	modpath.ln \
	modinit.ln \
	mod_exec.ln \
	mod_ksym.ln

all:	$(MOD) $(MODKSYM)

install: all
	(cd modksym.cf; $(IDINSTALL) -M -R$(CONF) modksym)
	(cd mod.cf; $(IDINSTALL) -M -R$(CONF) mod)

$(MOD):	$(FILES)
	$(LD) -r -o $(MOD) $(FILES)

$(MODKSYM):	mod_ksym.o modinit.o
	$(LD) -r -o $(MODKSYM) mod_ksym.o modinit.o

modctl.o:
	$(CC) -Xa $(INCLIST) $(DEFLIST) -c $<


clean:
	-rm -f *.o $(LFILES) *.L $(MOD) $(MODKSYM)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d modksym
	$(IDINSTALL) -e -R$(CONF) -d mod

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
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	ksym.h \
	mod.h \
	mod_intr.h \
	mod_k.h \
	mod_obj.h \
	moddefs.h \
	moddrv.h \
	modexec.h \
	modfs.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
