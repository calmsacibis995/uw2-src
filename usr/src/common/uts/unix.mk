#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:unix.mk	1.33"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	unix.mk
KBASE = .
LINTDIR = $(KBASE)/lintdir
DIR =

KERNEL = kernel.cf/Driver.o

CFDIR = $(CONF)/cf.d
CFFILES = $(CFDIR)/kernmap $(CFDIR)/res_major $(CFDIR)/deflist \
	  $(CFDIR)/type $(CONF)/interface.d

DKTEST =
KTESTROOT = .

SUBDIRS = $(DKTEST) psm acc fs io mem proc svc util net

IDBUILD = idbuild

all:	local FRC
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done
	@echo === unix.mk '"all"' completed at `date`

.MUTEX:	assym_hdrs $(KERNEL)

local:	assym_hdrs $(KERNEL)

assym_hdrs:	util/assym.h util/assym_dbg.h

$(KERNEL): svc/syms.o
	$(LD) -r -o $(KERNEL) svc/syms.o

svc/syms.o:	assym_hdrs
	@echo "=== $(MAKE) -f svc.mk syms.o"
	@cd svc; $(MAKE) -f svc.mk syms.o $(MAKEARGS)

util/assym.h:
	@echo "=== $(MAKE) -f util.mk assym.h"
	@cd util; $(MAKE) -f util.mk assym.h $(MAKEARGS)

util/assym_dbg.h:
	@echo "=== $(MAKE) -f util.mk assym_dbg.h"
	@cd util; $(MAKE) -f util.mk assym_dbg.h $(MAKEARGS)

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) install: KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done
	@echo === kernel installed at `date`

.MUTEX: local $(CFDIR) cffiles

localinstall:	local $(CFDIR) cffiles FRC
	cd kernel.cf; $(IDINSTALL) -R$(CONF) -M kernel

cffiles:	$(CFFILES)

$(CFDIR)/kernmap:	$(CFDIR) kernmap
	$(INS) -f $(CFDIR) -u $(OWN) -g $(GRP) kernmap

$(CFDIR)/res_major:	$(CFDIR) res_major
	$(INS) -f $(CFDIR) -u $(OWN) -g $(GRP) res_major

$(CFDIR)/deflist:	$(CFDIR) FRC
	echo "$(DEFLIST)" | sed -e 's/ -D_KERNEL_HEADERS//' >$(CFDIR)/deflist

$(CONF)/interface.d:	$(CONF) FRC
	[ -d $(CONF)/interface.d ] || mkdir -p $(CONF)/interface.d
	-rm -f $(CONF)/interface.d/*
	cp interface.d/* $(CONF)/interface.d

$(CFDIR)/type:	$(CFDIR) FRC
	@UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		echo "atup" >$(CFDIR)/type; \
	else \
		echo "mp" >$(CFDIR)/type; \
	fi

$(CFDIR):
	-mkdir -p $@

.MUTEX:	install $(IDBUILD)
.MUTEX:	headinstall $(IDBUILD)

build:	install headinstall $(IDBUILD)

$(IDBUILD):
	ROOT=$(ROOT) PFX=$(PFX) MACH=$(MACH) $(IDBUILD) -K
	@echo === kernel built at `date`

depend:	assym_hdrs
	@find $(SUBDIRS) -name "*.mk" -print |\
		while read i; do\
			(if [ -f $$i.dep ];\
		 	then\
				: ;\
		 	else\
				echo "" >$$i.dep;\
			fi);\
		done
	@for d in $(SUBDIRS); do\
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done
	@echo === unix.mk '"depend"' completed at `date`

clean:	localclean
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) clean: KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f util/*assym*.h $(KERNEL)
	-rm -rf $(LINTDIR)

localclobber:	localclean
	-rm -rf $(CFFILES)
	-$(IDINSTALL) -R$(CONF) -d -e kernel

clobber: localclobber
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) clobber: KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

lintit: $(LINTDIR) FRC
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) : KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done
	cd $(LINTDIR); \
	cat *.L > lint.out; \
	echo "============ combined output ========" >> lint.out; \
	$(LINT) $(LINTFLAGS) *.ln >> lint.out

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 $(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		 $(SED) -e "s;^;$$d/;"); \
	done

headinstall:
	@for d in $(SUBDIRS); do \
		(if [ "$$d" = ktest ]; \
		  then \
			echo "$(MAKE) headinstall: KTEST included!"; \
			cd $(KTESTROOT)/$$d; \
		  else \
			cd $$d; \
		 fi; \
		 echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

FRC:
