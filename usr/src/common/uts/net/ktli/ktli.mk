#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/ktli/ktli.mk	1.10"
#ident	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

include $(UTSRULES)

# Connection oriented ktli is not supported yet, but
# t_kconnect() exists. we do not compile it.
#LOCALDEF = -DKTLICONN

MAKEFILE=	ktli.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/ktli

KTLI = ktli.cf/Driver.o
LFILE = $(LINTDIR)/ktli.ln

MODULES = \
	$(KTLI)

FILES = t_kclose.o \
	t_kgtstate.o \
	t_ksndudat.o \
	t_kutil.o \
	t_kalloc.o \
	t_kconnect.o \
	t_kopen.o \
	t_kspoll.o \
	t_kbind.o \
	t_kunbind.o \
	t_kfree.o \
	t_krcvudat.o 

LFILES = t_kclose.ln \
	t_kgtstate.ln \
	t_ksndudat.ln \
	t_kutil.ln \
	t_kalloc.ln \
	t_kconnect.ln \
	t_kopen.ln \
	t_kspoll.ln \
	t_kbind.ln \
	t_kunbind.ln \
	t_kfree.ln \
	t_krcvudat.ln 

CFILES = t_kclose.c \
	t_kgtstate.c \
	t_ksndudat.c \
	t_kutil.c \
	t_kalloc.c \
	t_kconnect.c \
	t_kopen.c \
	t_kspoll.c \
	t_kbind.c \
	t_kunbind.c \
	t_kfree.c \
	t_krcvudat.c 

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd ktli.cf; $(IDINSTALL) -R$(CONF) -M ktli)

$(KTLI): $(FILES)
	$(LD) -r -o $(KTLI) $(FILES)

clean:
	rm -f *.o $(LFILES) *.L $(KTLI)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ktli

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
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

sysHeaders = \
	t_kuser.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done



include $(UTSDEPEND)

include $(MAKEFILE).dep
