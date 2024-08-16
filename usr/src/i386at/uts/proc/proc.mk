#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:proc/proc.mk	1.31"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	proc.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = proc

PROC = proc.cf/Driver.o
LFILE = $(LINTDIR)/proc.ln

FILES = \
	acct.o \
	core.o \
	cred.o \
	cswtch.o \
	disp.o \
	exec.o \
	execseg.o \
	exit.o \
	iobitmap.o \
        pid.o \
        fork.o \
        procsubr.o \
        sig.o \
        sigcalls.o \
        sigmdep.o \
        scalls.o \
        pgrp.o \
        session.o \
        trapevt.o \
        rendez.o \
	resource.o \
	resource_f.o \
	uidquota.o \
	execargs.o \
	execmdep.o \
	dispmdep.o \
	priocntl.o \
	procset.o \
	bind.o \
	lwpsubr.o \
	grow.o \
	lwpscalls.o \
	procmdep.o \
	seize.o \
	usync.o

CFILES = \
	acct.c \
	core.c \
	cred.c \
	disp.c \
	exec.c \
	execseg.c \
	exit.c \
	iobitmap.c \
	pid.c \
	fork.c \
	procsubr.c \
	uidquota.c \
        sig.c \
        sigcalls.c \
        sigmdep.c \
        scalls.c \
        pgrp.c \
        session.c \
        trapevt.c \
        rendez.c \
	resource.c \
	resource_f.c \
	dispmdep.c \
	priocntl.c \
	procset.c \
	bind.c \
	lwpsubr.c \
	grow.c \
	lwpscalls.c \
	procmdep.c \
	seize.c \
	usync.c

SFILES = \
	cswtch.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	acct.ln \
	core.ln \
	cred.ln \
	disp.ln \
	exec.ln \
	execseg.ln \
	exit.ln \
	iobitmap.ln \
        pid.ln \
        fork.ln \
        procsubr.ln \
        sig.ln \
        sigcalls.ln \
        sigmdep.ln \
        scalls.ln \
        pgrp.ln \
        session.ln \
        trapevt.ln \
        rendez.ln \
	resource.ln \
	resource_f.ln \
	uidquota.ln \
	execargs.ln \
	execmdep.ln \
	dispmdep.ln \
	priocntl.ln \
	procset.ln \
	bind.ln \
	lwpsubr.ln \
	grow.ln \
	lwpscalls.ln \
	procmdep.ln \
	seize.ln \
	usync.ln

SUBDIRS = class obj ipc

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(PROC)

$(PROC): $(FILES)
	$(LD) -r -o $(PROC) $(FILES)

cswtch.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

localinstall: local FRC
	cd proc.cf; $(IDINSTALL) -R$(CONF) -M proc

clean: localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(PROC) $(LFILES) *.L

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e proc

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SUBDIRS);\
	do\
		( \
		cd  $$i;\
		$(MAKE) -f $$i.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$i/;" ; \
		) \
	done
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	acct.h \
	auxv.h \
	bind.h \
	class.h \
	core.h \
	cred.h \
	disp.h \
	disp_p.h \
	exec.h \
	exec_f.h \
	iobitmap.h \
	lwp_f.h \
	lwp.h \
	mman.h \
	pid.h \
	priocntl.h \
	proc.h \
	procset.h \
	resource.h \
	regset.h \
	seg.h \
	session.h \
	siginfo.h \
	signal.h \
	times.h \
	tss.h \
	ucontext.h \
	uidquota.h \
	ulimit.h \
	user.h \
	usync.h \
	unistd.h \
	wait.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep

